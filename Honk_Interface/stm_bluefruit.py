# Example of interaction with a BLE UART device using a UART service
# implementation.
# Author: Tony DiCola
# import atexit
import time
from typing import Any, Callable

import Adafruit_BluefruitLE
from Adafruit_BluefruitLE.services import UART

HEAD = b"\xCC"
TAIL = b"\xB9"
END1 = b"\x0D"
END2 = b"\x0A"

MAX_DATA_LEN = 127

DEBUG_SEND = False
DEBUG_RECV = False
DEBUG_RECV_DROP = False

# Get the BLE provider for the current platform.
ble = Adafruit_BluefruitLE.get_provider()

adapter = None
devices = list()
uarts = list()
in_packet_ids = list()
out_packet_ids = list()


def get_data_bytes(data: tuple[int, str]):
    id, data = data

    cksum = id
    for byte in data.encode("utf-8"):
        cksum ^= byte
    return (
        HEAD
        + (len(data)).to_bytes(1, "big")
        + id.to_bytes(1, "big")
        # + (packet_id - 1).to_bytes(1, "big")
        + data.encode("utf-8")
        + TAIL
        + cksum.to_bytes(1, "big")
        + END1
        + END2
    )


tmp_id = None
tmp_len = 0
tmp_data = b""
tmp_cksum = 0
state = 0
data_index = 0
ready = False


def parse_packet(input):
    global tmp_id, tmp_len, tmp_data, tmp_cksum, state, data_index, ready
    if state == 0:
        if input == int.from_bytes(HEAD):
            state = 1
            tmp_id = None
            tmp_len = 0
            tmp_data = []
            tmp_cksum = 0
            data_index = 0
            ready = False
    elif state == 1:
        tmp_len = input
        state = 2
    elif state == 2:
        tmp_id = input
        tmp_cksum = tmp_id
        if tmp_len == 0:
            state = 4
        else:
            state = 3
    elif state == 3:
        tmp_data += [int.to_bytes(input, 1, "big")]
        tmp_cksum ^= input
        data_index += 1
        if data_index == tmp_len:
            state = 4
    elif state == 4:
        if input == int.from_bytes(TAIL):
            state = 5
        else:
            state = 0
    elif state == 5:
        if input == tmp_cksum:
            state = 6
        else:
            state = 0
    elif state == 6:
        if input == int.from_bytes(END1):
            state = 7
        else:
            state = 0
    elif state == 7:
        if input == int.from_bytes(END2):
            ready = True
            state = 0
        else:
            state = 0


def debug_modes(
    send: bool = DEBUG_SEND, recv: bool = DEBUG_RECV, recv_drop: bool = DEBUG_RECV_DROP
):
    global DEBUG_SEND, DEBUG_RECV, DEBUG_RECV_DROP
    DEBUG_SEND = send
    DEBUG_RECV = recv
    DEBUG_RECV_DROP = recv_drop


def init(count: int = 0) -> int:
    global ble, adapter, devices, uarts, in_packet_ids, out_packet_ids
    # Clear any cached data because both bluez and CoreBluetooth have issues with
    # caching data and it going stale.
    ble.clear_cached_data()

    # Get the first available BLE network adapter and make sure it's powered on.
    adapter = ble.get_default_adapter()
    adapter.power_on()
    print("Using adapter: {0}".format(adapter.name))

    # Disconnect any currently connected UART devices.  Good for cleaning up and
    # starting from a fresh state.
    print("Disconnecting any connected UART devices...")
    UART.disconnect_devices()

    # Scan for UART devices.
    print("Searching for UART device...")
    try:
        adapter.start_scan()
        # Search for the first UART device found (will time out after 60 seconds
        # but you can specify an optional timeout_sec parameter to change it).
        known = set()
        if count == 0:
            for _ in range(5):
                found = set(UART.find_devices())
                new = found - known
                for device in new:
                    print(f"Found UART {len(known)}: {device.name} [{device.id}]")
                    known.add(device)
                # known.update(new)
                time.sleep(1.0)
            if len(known) == 0:
                raise RuntimeError("Failed to find any UART devices!")
        else:
            start_time = time.time()
            while len(known) < count and time.time() - start_time < 10:
                found = set(UART.find_devices())
                new = found - known
                for device in new:
                    print(f"Found UART {len(known)}: {device.name} [{device.id}]")
                    known.add(device)
                time.sleep(1.0)
            if len(known) < count:
                raise RuntimeError(f"Failed to find {count} UART devices!")
        devices = list(known)
    finally:
        # Make sure scanning is stopped before exiting.
        adapter.stop_scan()

    print("Connecting to device...")
    for device in devices:
        device.connect()  # Will time out after 60 seconds, specify timeout_sec parameter
    # to change the timeout.

    try:
        # Wait for service discovery to complete for the UART service.  Will
        # time out after 60 seconds (specify timeout_sec parameter to override).
        print("Discovering services...")
        for device in devices:
            UART.discover(device)
            uarts.append(UART(device))
            in_packet_ids.append(0)
            out_packet_ids.append(0)

        return len(uarts)

    except Exception as e:
        print("Error: {0}".format(e))
        for device in devices:
            device.disconnect()
        raise


def recv() -> list[list[str]]:
    global tmp_id, tmp_data, ready, uarts, in_packet_ids
    in_queue = [list() for _ in uarts]
    try:
        for i, uart in enumerate(uarts):
            # Now wait up to one second to receive data from the device.
            received = uart.read(timeout_sec=1)
            if received is not None:
                for byte in received:
                    parse_packet(byte)
                    if ready:
                        in_queue[i].append(
                            "".join(
                                [
                                    (
                                        ""
                                        if int.from_bytes(c) >= 128
                                        else c.decode("utf-8")
                                    )
                                    for c in tmp_data
                                ]
                            )
                        )
                        if DEBUG_RECV:
                            print("Received: {0}, UART: {1}".format(in_queue[i][-1], i))
                        if tmp_id != in_packet_ids[i] and DEBUG_RECV_DROP:
                            print(
                                "Packet ID mismatch: {0} packets dropped".format(
                                    (tmp_id - in_packet_ids[i] + 256) % 256
                                )
                            )
                        ready = False
                        in_packet_ids[i] = (tmp_id + 1) % 256
        return in_queue
    except Exception as e:
        print("Error: {0}".format(e))
        for device in devices:
            device.disconnect()
        raise


def send(uart_id: int, data: str) -> bool:
    global uarts, out_packet_ids

    if len(data) > MAX_DATA_LEN:
        return False

    if DEBUG_SEND:
        print(
            "Sending: {0}: {1}, UART: {2}".format(
                out_packet_ids[uart_id], data, uart_id
            )
        )
    uarts[uart_id].write(get_data_bytes((out_packet_ids[uart_id], data)))
    out_packet_ids[uart_id] = (out_packet_ids[uart_id] + 1) % 256
    return True


def run(f: Callable[[], Any]):
    # Initialize the BLE system.  MUST be called before other BLE calls!
    ble.initialize()

    # Start the mainloop to process BLE events, and run the provided function in
    # a background thread.  When the provided main function stops running, returns
    # an integer status code, or throws an error the program will exit.
    ble.run_mainloop_with(f)


if __name__ == "__main__":
    # Main function implements the program logic so it can run in a background
    # thread.  Most platforms require the main thread to handle GUI events and other
    # asyncronous events like BLE actions.  All of the threading logic is taken care
    # of automatically though and you just need to provide a main function that uses
    # the BLE provider.
    def test():
        alphabet = "abcdefghijklmnopqrstuvwxyz"

        device_count = init()
        debug_modes(True, True, True)

        alphabet_idx = [0 for _ in range(device_count)]
        packet_times = [list() for _ in range(device_count)]
        packet_drops = [0 for _ in range(device_count)]
        last_sent = ["" for _ in range(device_count)]
        last_sent_time = [None for _ in range(device_count)]
        transaction = [False for _ in range(device_count)]

        DROP_THRESHOLD = 0.2  # seconds

        # Once connected do everything else in a try/finally to make sure the device
        # is disconnected when done.

        for _ in range(100):
            queue = recv()
            for i in range(device_count):
                transaction[i] = False
                for data in queue[i]:
                    if data == last_sent[i]:
                        alphabet_idx[i] += 1
                        if alphabet_idx[i] == len(alphabet):
                            alphabet_idx[i] = 0

                        last_sent[i] = alphabet[
                            alphabet_idx[i] : (min(alphabet_idx[i] + 5, len(alphabet)))
                        ]
                        send(
                            i,
                            f"{last_sent[i]}",
                        )
                        transaction[i] = True
                        packet_times[i].append(time.time() - last_sent_time[i])
                        last_sent_time[i] = time.time()

                if not transaction[i] and (
                    last_sent_time[i] is None
                    or time.time() - last_sent_time[i] > DROP_THRESHOLD
                ):
                    if last_sent[i] == "":
                        last_sent[i] = alphabet[
                            alphabet_idx[i] : (min(alphabet_idx[i] + 5, len(alphabet)))
                        ]

                    send(
                        i,
                        f"{last_sent[i]}",
                    )
                    transaction[i] = True
                    packet_drops[i] += 1
                    last_sent_time[i] = time.time()

        for i in range(device_count):
            print(
                f"UART: {i} - Avg Packet Time: {round(sum(packet_times[i]) / len(packet_times[i]), 3)}s, Dropped: {packet_drops[i]}, Total Attempts: {len(packet_times[i]) + packet_drops[i]}, Drop Rate: {round(100 * packet_drops[i] / (len(packet_times[i]) + packet_drops[i]), 3)}%"
            )

    run(test)

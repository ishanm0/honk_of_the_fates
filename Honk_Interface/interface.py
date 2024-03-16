# Example of interaction with a BLE UART device using a UART service
# implementation.
# Author: Tony DiCola
# import atexit
import time

import Adafruit_BluefruitLE
from Adafruit_BluefruitLE.services import UART

HEAD = b"\xCC"
TAIL = b"\xB9"
END1 = b"\x0D"
END2 = b"\x0A"

DEBUG_SEND = False
DEBUG_RECV = False

# Get the BLE provider for the current platform.
ble = Adafruit_BluefruitLE.get_provider()

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


# Main function implements the program logic so it can run in a background
# thread.  Most platforms require the main thread to handle GUI events and other
# asyncronous events like BLE actions.  All of the threading logic is taken care
# of automatically though and you just need to provide a main function that uses
# the BLE provider.
def main():
    global tmp_id, tmp_data, ready  # , packet_id
    alphabet = "abcdefghijklmnopqrstuvwxyz\n"
    j = 0
    count = 0

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
        for _ in range(5):
            found = set(UART.find_devices())
            new = found - known
            for device in new:
                print("Found UART: {0} [{1}]".format(device.name, device.id))
            known.update(new)
            time.sleep(1.0)
        if len(known) == 0:
            raise RuntimeError("Failed to find any UART devices!")
        devices = list(known)
    finally:
        # Make sure scanning is stopped before exiting.
        adapter.stop_scan()

    print("Connecting to device...")
    for device in devices:
        device.connect()  # Will time out after 60 seconds, specify timeout_sec parameter
    # to change the timeout.

    # Once connected do everything else in a try/finally to make sure the device
    # is disconnected when done.
    try:
        # Wait for service discovery to complete for the UART service.  Will
        # time out after 60 seconds (specify timeout_sec parameter to override).
        print("Discovering services...")
        for device in devices:
            UART.discover(device)
            uarts.append(UART(device))
            in_packet_ids.append(0)
            out_packet_ids.append(0)

        # Once service discovery is complete create an instance of the service
        # and start interacting with it.

        in_queues = [list() for _ in uarts]
        out_queues = [list() for _ in uarts]

        while True:
            for i, uart in enumerate(uarts):
                # Now wait up to one minute to receive data from the device.
                received = uart.read(timeout_sec=1)
                if received is not None:
                    for byte in received:
                        parse_packet(byte)
                        if ready:
                            in_queues[i].append(
                                (
                                    tmp_id,
                                    "".join(
                                        [
                                            (
                                                ""
                                                if int.from_bytes(c) >= 128
                                                else c.decode("utf-8")
                                            )
                                            for c in tmp_data
                                        ]
                                    ),
                                )
                            )
                            if tmp_id != in_packet_ids[i]:
                                print(
                                    "Packet ID mismatch: {0} != {1}".format(
                                        tmp_id, in_packet_ids[i]
                                    )
                                )
                            ready = False
                            in_packet_ids[i] = (tmp_id + 1) % 256

                # TESTING: handle incoming packets, send outgoing packets
                while len(in_queues[i]) > 0:
                    id, data = in_queues[i].pop(0)
                    if DEBUG_RECV:
                        print("Received: {0}: {1}, UART: {2}".format(id, data, i))

                out_queues[i].append(
                    (out_packet_ids[i], f"{alphabet[j:(min(j+5, len(alphabet)))]}")
                )
                out_packet_ids[i] = (out_packet_ids[i] + 1) % 256

                while len(out_queues[i]) > 0:
                    to_send = out_queues[i][0]
                    if DEBUG_SEND:
                        print(
                            "Sending: {0}: {1}, UART: {2}".format(
                                to_send[0], to_send[1], i
                            )
                        )
                    uart.write(get_data_bytes(out_queues[i].pop(0)))
                    time.sleep(0.5)

            j += 1
            if j == len(alphabet):
                j = 0
            count += 1
    finally:
        # Make sure device is disconnected on exit.
        device.disconnect()


# Initialize the BLE system.  MUST be called before other BLE calls!
ble.initialize()

# Start the mainloop to process BLE events, and run the provided function in
# a background thread.  When the provided main function stops running, returns
# an integer status code, or throws an error the program will exit.
ble.run_mainloop_with(main)

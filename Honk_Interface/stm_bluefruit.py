# Example of interaction with a BLE UART device using a UART service
# implementation.
# Author: Tony DiCola, Ishan Madan, India Spott
import time
from typing import Any, Callable

import Adafruit_BluefruitLE
from Adafruit_BluefruitLE.services import UART

# Define special chars used in the protocol
HEAD = b"\xCC"
TAIL = b"\xB9"
END1 = b"\x0D"
END2 = b"\x0A"

# max supported data length
MAX_DATA_LEN = 127
MAX_PACKET_ID = 255

# Debug print flags
DEBUG_SEND = False
DEBUG_RECV = False
DEBUG_RECV_DROP = False

# Get the BLE provider for the current platform.
ble = Adafruit_BluefruitLE.get_provider()

adapter = None  # bluetooth adapter
devices = list()  # list of available ble devices
uarts = list()  # list of uart services
in_packet_ids = list()  # id of expected incoming packets for each uart
out_packet_ids = list()  # id of next outgoing packet for each uart


# Convert packet id and text to byte array (message to send)
def get_data_bytes(data: tuple[int, str]):
    id, text = data

    cksum = id
    for byte in text.encode("utf-8"):
        cksum ^= byte
    return (
        HEAD
        + (len(text)).to_bytes(1, "big")
        + id.to_bytes(1, "big")
        # + (packet_id - 1).to_bytes(1, "big")
        + text.encode("utf-8")
        + TAIL
        + cksum.to_bytes(1, "big")
        + END1
        + END2
    )


# temp variables for packet parsing
tmp_id = None  # packet id
tmp_len = 0  # packet length
tmp_data = []  # message data (content)
tmp_cksum = 0  # message checksum
state = 0  # parsing state (see ECE 121 :)
data_index = 0  # index of current byte in message data
ready = False  # message is ready to be processed


# state machine to parse incoming packets (see ECE 121 :)
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


# Set debug print flags
def debug_modes(
    send: bool = DEBUG_SEND, recv: bool = DEBUG_RECV, recv_drop: bool = DEBUG_RECV_DROP
):
    global DEBUG_SEND, DEBUG_RECV, DEBUG_RECV_DROP
    DEBUG_SEND = send
    DEBUG_RECV = recv
    DEBUG_RECV_DROP = recv_drop


# Initialize the BLE system and connect to the UART devices, providing an expected number of devices (0 for auto-detect)
def init(count: int = 0) -> int:
    global ble, adapter, devices, uarts, in_packet_ids, out_packet_ids
    # Clear any cached data because both bluez and CoreBluetooth have issues with
    # caching data and it going stale.
    ble.clear_cached_data()

    # Get the first available BLE network adapter and make sure it's powered on.
    adapter = ble.get_default_adapter()
    if adapter is None:
        raise RuntimeError("No BLE adapter found.")
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

        # Keep track of found devices so that we don't re-add the them
        known = set()

        if count == 0:
            # If count is 0, auto-detect devices (5 scans, each 1 second apart)
            for _ in range(5):
                # find all devices
                found = set(UART.find_devices())

                # identify which devices are new
                new = found - known
                for device in new:
                    # print new devices
                    print(f"Found UART {len(known)}: {device.name} [{device.id}]")

                    # add new devices to known devices
                    known.add(device)

                # wait for 1 second between scans
                time.sleep(1.0)

            # if no devices are found, raise an error
            if len(known) == 0:
                raise RuntimeError("Failed to find any UART devices!")
        else:
            # if count is not 0, scan until expected # of devices are found, or 10 seconds have passed
            start_time = time.time()
            while len(known) < count and time.time() - start_time < 10:
                # find all devices
                found = set(UART.find_devices())

                # identify which devices are new
                new = found - known
                for device in new:
                    # print new devices
                    print(f"Found UART {len(known)}: {device.name} [{device.id}]")

                    # add new devices to known devices
                    known.add(device)

                # wait for 1 second between scans
                time.sleep(1.0)

            # if not enough devices are found, raise an error
            if len(known) < count:
                raise RuntimeError(f"Failed to find {count} UART devices!")

        # convert known devices to a list
        devices = list(known)
    finally:
        # Make sure scanning is stopped before exiting.
        adapter.stop_scan()

    # Connect to all found devices
    print("Connecting to devices...")
    for device in devices:
        device.connect()  # Will time out after 60 seconds, specify timeout_sec parameter
    # to change the timeout.

    try:
        # Wait for service discovery to complete for the UART service.  Will
        # time out after 60 seconds (specify timeout_sec parameter to override).
        print("Discovering services...")
        for device in devices:
            # discover the UART service and add it to the list of UART services, and initialize packet IDs
            UART.discover(device)
            uarts.append(UART(device))
            in_packet_ids.append(0)
            out_packet_ids.append(0)

        # Return the number of connected devices
        return len(uarts)

    except Exception as e:
        print("Error: {0}".format(e))

        # upon error, disconnect all devices and raise the error
        for device in devices:
            device.disconnect()
        raise


# Receive data from all connected devices
def recv() -> list[list[str]]:
    global tmp_id, tmp_data, ready, uarts, in_packet_ids

    # Initialize a list of incoming data queues for each UART
    in_queue = [list() for _ in uarts]
    try:
        # Loop through all connected devices
        for i, uart in enumerate(uarts):
            # Now wait up to 0.1 seconds to receive data from the device.
            received = uart.read(timeout_sec=0.1)

            # if data is received
            if received is not None:
                # Send each received byte through the packet parser state machine
                for byte in received:
                    parse_packet(byte)

                    # If a message is ready, decode it from bytes to str and add it to the incoming data queue
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

                        # debug print
                        if DEBUG_RECV:
                            print("Received: {0}, UART: {1}".format(in_queue[i][-1], i))

                        # if packet ID is not as expected, print a warning - some number of packets may have been dropped
                        if tmp_id != in_packet_ids[i] and DEBUG_RECV_DROP:
                            if tmp_id is not None:
                                print(
                                    "Packet ID mismatch: {0} packets dropped".format(
                                        (
                                            tmp_id
                                            - in_packet_ids[i]
                                            + (MAX_PACKET_ID + 1)
                                        )
                                        % (MAX_PACKET_ID + 1)
                                    )
                                )
                            else:
                                print("Error: Packet ID is None")

                        # mark the packet as processed
                        ready = False

                        # update the expected packet ID
                        if tmp_id is not None:
                            in_packet_ids[i] = (tmp_id + 1) % (MAX_PACKET_ID + 1)

        # return the incoming data queue
        return in_queue
    except Exception as e:
        print("Error: {0}".format(e))
        for device in devices:
            device.disconnect()
        raise


# Send data to a specific device
def send(uart_id: int, data: str) -> bool:
    global uarts, out_packet_ids

    # Check if the data is too long
    if len(data) > MAX_DATA_LEN:
        return False

    # debug print
    if DEBUG_SEND:
        print(
            "Sending: {0}: {1}, UART: {2}".format(
                out_packet_ids[uart_id], data, uart_id
            )
        )

    # Send the data to the device (after converting it to bytes)
    uarts[uart_id].write(get_data_bytes((out_packet_ids[uart_id], data)))

    # update the packet ID
    out_packet_ids[uart_id] = (out_packet_ids[uart_id] + 1) % (MAX_PACKET_ID + 1)
    return True


# Main function to run the provided function in a background thread - required for BLE
def run(f: Callable[[], Any]):
    # Initialize the BLE system.  MUST be called before other BLE calls!
    ble.initialize()

    # Start the mainloop to process BLE events, and run the provided function in
    # a background thread.  When the provided main function stops running, returns
    # an integer status code, or throws an error the program will exit.
    ble.run_mainloop_with(f)


if __name__ == "__main__":
    # Test function to send a sequence of packets to all connected devices, measure the time between packets, count dropped packets, and resend packets that have not been echoed back
    def test():
        alphabet = "abcdefghijklmnopqrstuvwxyz"

        # Initialize the BLE system and connect to the UART devices
        device_count = init()

        # Set debug print flags
        debug_modes(True, True, True)

        alphabet_idx = [
            0 for _ in range(device_count)
        ]  # index of next characters to send for each device
        packet_times = [
            list() for _ in range(device_count)
        ]  # time taken to receive each echoed packet for each device
        packet_drops = [
            0 for _ in range(device_count)
        ]  # number of dropped packets for each device
        last_sent = [
            "" for _ in range(device_count)
        ]  # last sent packet for each device
        last_sent_time = [
            0.0 for _ in range(device_count)
        ]  # time of last sent packet for each device
        transaction = [
            False for _ in range(device_count)
        ]  # flag to indicate if a packet has been received for each device

        DROP_THRESHOLD = 0.2  # seconds

        # run for 1000 iterations
        for _ in range(1000):
            # Receive data from all connected devices
            queue = recv()

            # For each device
            for i in range(device_count):
                # reset transaction flag
                transaction[i] = False

                # for each received packet for this device
                for data in queue[i]:
                    # if the received packet is the expected echo of the last sent packet
                    if data == last_sent[i]:
                        # increment the alphabet index (and wrap around if necessary)
                        alphabet_idx[i] += 1
                        if alphabet_idx[i] == len(alphabet):
                            alphabet_idx[i] = 0

                        # update the next packet to send
                        last_sent[i] = alphabet[
                            alphabet_idx[i] : (min(alphabet_idx[i] + 5, len(alphabet)))
                        ]

                        # send the next packet
                        send(
                            i,
                            f"{last_sent[i]}",
                        )

                        # mark the transaction as complete
                        transaction[i] = True

                        # record the time taken to receive the packet and the time this packet was sent at
                        packet_times[i].append(time.time() - last_sent_time[i])
                        last_sent_time[i] = time.time()

                # if a packet has not been received for this device within the DROP_THRESHOLD or if no packet has been sent
                if not transaction[i] and (
                    last_sent_time[i] == 0.0
                    or time.time() - last_sent_time[i] > DROP_THRESHOLD
                ):
                    # if no packet has been sent, send the first packet
                    if last_sent[i] == "":
                        last_sent[i] = alphabet[
                            alphabet_idx[i] : (min(alphabet_idx[i] + 5, len(alphabet)))
                        ]

                    # send the first packet or resend the last packet
                    send(
                        i,
                        f"{last_sent[i]}",
                    )

                    # mark the transaction as complete
                    transaction[i] = True

                    # increment the number of dropped packets
                    packet_drops[i] += 1

                    # record the time this packet was sent at
                    last_sent_time[i] = time.time()

        # print metrics for each device
        for i in range(device_count):
            print(
                f"UART: {i} - Avg Packet Time: {round(sum(packet_times[i]) / len(packet_times[i]), 3)}s, Dropped: {packet_drops[i]}, Total Attempts: {len(packet_times[i]) + packet_drops[i]}, Drop Rate: {round(100 * packet_drops[i] / (len(packet_times[i]) + packet_drops[i]), 3)}%"
            )

    run(test)

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

# Get the BLE provider for the current platform.
ble = Adafruit_BluefruitLE.get_provider()

devices = list()
uarts = list()
packet_ids = list()


# def get_data_bytes(id: int, data: str):
#     out = HEAD + id.to_bytes(1, "big") + data.encode("utf-8") + TAIL + END1 + END2
#     return out + (b"\x00" * ((12 if len(out) < 128 else 256) - len(out)))


# def get_data_bytes(id: int, data: bytes):
#     out = HEAD + id.to_bytes(1, "big") + data + TAIL + END1 + END2
#     return out + (b"\x00" * ((128 if len(out) < 128 else 256) - len(out)))


# def get_data_bytes(data: tuple[int, bytes]):
#     id, data = data
#     out = HEAD + id.to_bytes(1, "big") + data + TAIL + END1 + END2
#     # return out
#     return out + (b"\x00" * ((128 if len(out) < 128 else 256) - len(out)))


def get_data_bytes(data: tuple[int, str]):
    # global packet_id
    id, data = data

    # make data 16 bytes long
    # data = data + " " * (16 - len(data))

    cksum = id
    for byte in data.encode("utf-8"):
        cksum ^= byte
    # packet_id += 1
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
    # return out
    # return out + (b"\xff" * ((26 if len(out) < 126 else 254) - len(out))) + END1 + END2


tmp_id = None
tmp_len = 0
tmp_data = b""
tmp_cksum = 0
state = 0
data_index = 0
ready = False


def parse_packet(input):
    global tmp_id, tmp_len, tmp_data, tmp_cksum, state, data_index, ready
    # print(input, hex(input))
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
    # alphabet = "".join(reversed("abcdefghijklmnopqrstuvwxyz"))
    alphabet = "abcdefghijklmnopqrstuvwxyz\n"
    # print(alphabet)
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
        # device = UART.find_device()
        # if device is None:
        #     raise RuntimeError('Failed to find UART device!')
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
            packet_ids.append(0)

        # Once service discovery is complete create an instance of the service
        # and start interacting with it.
        # for device in devices:
        # uart = UART(device)

        # Write a string to the TX characteristic.
        # for uart in uarts:
        #     uart.write(b"Hello World!\r\n")
        #     print("Sent 'Hello world!' to the device.")

        in_queues = [list() for _ in uarts]
        out_queues = [list() for _ in uarts]
        for i in range(len(out_queues)):
            # out_queues[i].append((i, "Hello World!\n"))
            # uarts[i].write("hello, world!\n".encode("utf-8"))
            packet_ids[i] += 1

        while True:
            for i, uart in enumerate(uarts):
                # print("UART: {0}".format(i))
                # Now wait up to one minute to receive data from the device.
                # print('Waiting up to 60 seconds to receive data from the device...')
                received = uart.read(timeout_sec=1)
                if received is not None:
                    # Received data, print it out.
                    # print("Received: {0}".format(received))
                    for byte in received:
                        # print('b', byte, hex(byte))
                        parse_packet(byte)
                        if ready:
                            # print(tmp_data, [type(c) for c in tmp_data])
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
                            ready = False
                else:
                    # Timeout waiting for data, None is returned.
                    # print("Received no data on UART {0}!".format(i))
                    # break
                    pass

                while len(in_queues[i]) > 0:
                    id, data = in_queues[i].pop(0)
                    print("Received: {0}: {1}, UART: {2}".format(id, data, i))

                # if count % 60 == 0:
                out_queues[i].append((packet_ids[i], f"{alphabet[j:j+2]}"))
                packet_ids[i] = (packet_ids[i] + 1) % 256

                while len(out_queues[i]) > 0:
                    # print(
                    #     get_data_bytes(out_queues[i][0]),
                    #     len(get_data_bytes(out_queues[i][0])),
                    #     # packet_id,
                    # )
                    to_send = out_queues[i][0]
                    print(
                        "Sending: {0}: {1}, UART: {2}".format(to_send[0], to_send[1], i)
                    )
                    uart.write(get_data_bytes(out_queues[i].pop(0)))
                    time.sleep(0.1)
                    # print("Sent data to UART {0}.".format(i))

            # if count % 60 == 0:
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

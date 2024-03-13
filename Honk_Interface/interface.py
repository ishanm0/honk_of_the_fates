# Example of interaction with a BLE UART device using a UART service
# implementation.
# Author: Tony DiCola
# import atexit
import datetime
import time
import random

import Adafruit_BluefruitLE
from Adafruit_BluefruitLE.services import UART


# Get the BLE provider for the current platform.
ble = Adafruit_BluefruitLE.get_provider()

devices = list()
uarts = list()


# Main function implements the program logic so it can run in a background
# thread.  Most platforms require the main thread to handle GUI events and other
# asyncronous events like BLE actions.  All of the threading logic is taken care
# of automatically though and you just need to provide a main function that uses
# the BLE provider.
def main():
    alphabet = "abcdefghijklmnopqrstuvwxyz"
    j = 0
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

        # Once service discovery is complete create an instance of the service
        # and start interacting with it.
        # for device in devices:
        # uart = UART(device)

        # Write a string to the TX characteristic.
        for uart in uarts:
            uart.write(b"Hello World!\r\n")
            print("Sent 'Hello world!' to the device.")

        while True:
            for i, uart in enumerate(uarts):
                # if random.random() < 0.5:
                #     ts = datetime.datetime.now().isoformat()
                # uart.write(b"hi there\r\n")
                uart.write(b"%s\r\n" % alphabet[j].encode('utf-8'))
                    # uart.write(b"%s\r\n" % ts.encode('utf-8'))
                    # print("Sent '{1}' to UART {0}.".format(i, ts))
                # Now wait up to one minute to receive data from the device.
                # print('Waiting up to 60 seconds to receive data from the device...')
                received = uart.read(timeout_sec=5)
                if received is not None:
                    # Received data, print it out.
                    print("Received: {0}, UART: {1}".format(received, i))
                else:
                    # Timeout waiting for data, None is returned.
                    print("Received no data on UART {0}!".format(i))
                    # break
            j += 1
            if j == 26:
                j = 0
    finally:
        # Make sure device is disconnected on exit.
        device.disconnect()


# Initialize the BLE system.  MUST be called before other BLE calls!
ble.initialize()

# Start the mainloop to process BLE events, and run the provided function in
# a background thread.  When the provided main function stops running, returns
# an integer status code, or throws an error the program will exit.
ble.run_mainloop_with(main)

import time

import stm_bluefruit as bt


def main():
    alphabet = "abcdefghijklmnopqrstuvwxyz\n"
    alphabet_idx = 0

    device_count = bt.init()
    bt.debug_modes(True, True, True)

    # Once connected do everything else in a try/finally to make sure the device
    # is disconnected when done.
    while True:
        queue = bt.recv()
        for i in range(device_count):

            # TESTING: handle incoming packets, send outgoing packets
            for data in queue[i]:
                print(f"handled: {data}")

            bt.send(i, f"{alphabet[alphabet_idx:(min(alphabet_idx+5, len(alphabet)))]}")

        alphabet_idx += 1
        if alphabet_idx == len(alphabet):
            alphabet_idx = 0


if __name__ == "__main__":
    bt.run(main)

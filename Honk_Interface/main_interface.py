import random
import time

import stm_bluefruit as bt

ACK_str = b"\x01".decode("utf-8")
# ACK (rest of message will be identical to the original message being ACKed - the second char will be the original message’s first char)
# 0x01 [msg id, 1 char] (2 bytes)

AssignID_str = b"\x02".decode("utf-8")
# assign ID
# 0x02 [msg id, 1 char] [player id, 1 char] (3 bytes)
# (laptop → STM)

ShotSent_str = b"\x03".decode("utf-8")
# shot sent
# 0x03 [msg id, 1 char] [player id, 1 char] [btn #, 1 char] (4 bytes)
# (STM → laptop)

ShotRecv_str = b"\x04".decode("utf-8")
# shot received
# 0x04 [msg id, 1 char] [receiving player’s id, 1 char] [sending player’s id, 1 char] (4 bytes)
# (STM → laptop)

Win_str = b"\x05".decode("utf-8")
# win
# 0x05 [msg id, 1 char] [player id, 1 char] (3 bytes)
# (laptop → STM)

Loss_str = b"\x06".decode("utf-8")
# loss
# 0x06 [msg id, 1 char] [player id, 1 char] (3 bytes)
# (laptop → STM)

Color_str = b"\x07".decode("utf-8")
# color choice
# 0x07 [msg id, 1 char] [color name, unknown # of chars]
# (STM → laptop)

Connection_str = b"\x08".decode("utf-8")
# connection confirmation
# 0x08 [msg id, 1 char]
# (laptop → STM)


already_sent_ack = set()
waiting_to_recv_ack = None
next_msg_id = 0


def send_ack(player_id: int, msg_id: str):
    global already_sent_ack
    already_sent_ack.add(msg_id)
    bt.send(player_id, ACK_str + msg_id)


def recv_ack(player_id: int, msg_id: str):
    global waiting_to_recv_ack
    for j in range(len(waiting_to_recv_ack[player_id])):
        if waiting_to_recv_ack[player_id][j][0] == msg_id:
            waiting_to_recv_ack[player_id].pop(j)
            return


def send(id: int, msg_type: str, msg_str: str, existing_id: str = None):
    global waiting_to_recv_ack, next_msg_id
    if existing_id is None:
        # set the ID message as waiting for an ACK
        waiting_to_recv_ack[id].append(
            [
                int.to_bytes(next_msg_id, 1, "big").decode("utf-8"),  # msg_id
                time.time(),  # time_sent
                msg_type,  # msg_type
                msg_str,  # msg_str
            ]
        )
        # increment the next_msg_id
        next_msg_id = (next_msg_id + 1) % 256
        # send the ID message
        bt.send(
            id,  # player_id
            waiting_to_recv_ack[id][-1][2]  # msg_type
            + waiting_to_recv_ack[id][-1][0]  # msg_id
            + waiting_to_recv_ack[id][-1][3],  # msg_str
        )
    else:
        bt.send(
            id,  # player_id
            msg_type + existing_id + msg_str,
        )
        return


def main():
    global already_sent_ack, waiting_to_recv_ack
    print("Starting main_interface.py")
    print("Press Ctrl+C to exit")
    print("")
    print("Enter the number of expected players/Bluetooth devices:")
    device_count = int(input())

    # each player's waiting_to_recv_ack list will contain a list like this: [msg_id, time_sent, msg_type, msg_str]
    waiting_to_recv_ack = [list() for _ in range(device_count)]
    shot_sent_queue = [list() for _ in range(device_count)]
    shot_recv_queue = [list() for _ in range(device_count)]

    shot_pairings = list()
    points = {i: 0 for i in range(device_count)}

    colors = ["" for _ in range(device_count)]

    bt.init(device_count)
    bt.debug_modes(True, True, True)

    for i in range(device_count):
        send(i, Connection_str, "")

    while any([len(waiting_to_recv_ack[i]) > 0 for i in range(device_count)]):
        # check for any new messages
        queue = bt.recv()
        for i in range(device_count):
            for data in queue[i]:
                # if the message is an ACK
                if data[0] == ACK_str:
                    # remove the message from the waiting_to_recv_ack list
                    recv_ack(i, data[1])
                elif data[0] == Color_str:
                    if colors[i] == "":
                        colors[i] = data[2:]
                        print(f"Device {i} has color: {colors[i]}")
                    send_ack(i, data[1])

        # send any messages that need to be resent
        for i in range(device_count):
            for j in range(len(waiting_to_recv_ack[i])):
                if time.time() - waiting_to_recv_ack[i][j][1] > 1:
                    waiting_to_recv_ack[i][j][1] = time.time()
                    send(
                        i,
                        waiting_to_recv_ack[i][j][2],  # msg_type
                        waiting_to_recv_ack[i][j][3],  # msg_str
                        waiting_to_recv_ack[i][j][0],  # msg_id
                    )

    # TODO: get color from each device & send back ACKs
    while any([colors[i] == "" for i in range(device_count)]):
        # check for any new messages
        queue = bt.recv()
        for i in range(device_count):
            for data in queue[i]:
                if data[0] == Color_str and colors[i] == "":
                    colors[i] = data[2:]
                    print(f"Device {i} has color: {colors[i]}")
                send_ack(i, data[1])

    # assign IDs
    ids = []
    for i in range(device_count):
        # generate a random ID for each device
        tmp_id = random.randint(1, 8)
        while tmp_id in ids:
            tmp_id = random.randint(1, 8)

        # ids[device_index] = player_id
        ids.append(tmp_id)

        # send the ID message
        send(i, AssignID_str, int.to_bytes(ids[i], 1, "big").decode("utf-8"))

        print(f"Device {i} has ID: {ids[i]}")

    # wait for all ACKs to be received
    while any([len(waiting_to_recv_ack[i]) > 0 for i in range(device_count)]):
        # check for any new messages
        queue = bt.recv()
        for i in range(device_count):
            for data in queue[i]:
                # if the message is an ACK
                if data[0] == ACK_str:
                    # remove the message from the waiting_to_recv_ack list
                    recv_ack(i, data[1])

        # send any messages that need to be resent
        for i in range(device_count):
            for j in range(len(waiting_to_recv_ack[i])):
                if time.time() - waiting_to_recv_ack[i][j][1] > 1:
                    waiting_to_recv_ack[i][j][1] = time.time()
                    send(
                        i,
                        waiting_to_recv_ack[i][j][2],  # msg_type
                        waiting_to_recv_ack[i][j][3],  # msg_str
                        waiting_to_recv_ack[i][j][0],  # msg_id
                    )

    while True:
        # check for any new messages
        queue = bt.recv()
        recv_time = time.time()
        for i in range(device_count):
            for data in queue[i]:
                if data[0] == ShotSent_str:
                    if data[1] not in already_sent_ack:
                        shot_sent_queue[i].append(
                            (recv_time, data[3].encode("utf-8"))
                        )  # (time_sent, btn #)

                    send_ack(i, data[1])
                elif data[0] == ShotRecv_str:
                    if data[1] not in already_sent_ack:
                        if data[3].encode("utf-8") not in ids:
                            print(
                                f"Invalid player ID: {data[3].encode('utf-8')} received from player {i}!"
                            )
                            continue
                        shot_recv_queue[i].append(
                            (recv_time, ids.index(data[3].encode("utf-8")))
                        )  # (time_sent, sending player's ID)

                    send_ack(i, data[1])
                elif data[0] == ACK_str:
                    recv_ack(i, data[1])

        shots_to_discard = list()
        recvs_to_discard = list()

        for i in range(device_count):  # i is the receiving player's ID
            for j in range(len(shot_recv_queue[i])):
                # (i, j) will never be in recvs_to_discard bc we're iterating through those in order
                # if (i, j) in recvs_to_discard:
                #     continue
                recv_shot = shot_recv_queue[i][j]  # (time_sent, sending player's ID)
                for k in range(len(shot_sent_queue[recv_shot[1]])):
                    if (recv_shot[1], k) in shots_to_discard:
                        continue
                    sent_shot = shot_sent_queue[recv_shot[1]][k]  # (time_sent, btn #)
                    # if the time difference is less than 0.5 seconds
                    if abs(recv_shot[0] - sent_shot[0]) < 0.5:
                        # (receiving player's ID, sending player's ID, btn #)
                        shot_pairings.append(
                            (
                                (recv_shot[0] + sent_shot[0]) / 2,  # time
                                i,  # receiving player's ID
                                recv_shot[1],  # sending player's ID
                                sent_shot[1],  # btn #
                            )
                        )

                        recvs_to_discard.append((i, j))
                        shots_to_discard.append((recv_shot[1], k))

        for i, j in recvs_to_discard:
            shot_recv_queue[i].pop(j)
        for i, j in shots_to_discard:
            shot_sent_queue[i].pop(j)

        shot_pairings.sort(key=lambda x: x[0])
        handled_pairings = set()
        wins = list()
        losses = list()
        for i in range(len(shot_pairings)):
            for j in range(i + 1, len(shot_pairings)):
                if (
                    # pairings haven't been handled yet
                    i not in handled_pairings
                    and j not in handled_pairings
                    # players are the same
                    and shot_pairings[i][1] == shot_pairings[j][2]
                    and shot_pairings[i][2] == shot_pairings[j][1]
                    # times are close enough
                    and abs(shot_pairings[i][0] - shot_pairings[j][0]) < 0.5
                ):
                    if shot_pairings[i][3] == shot_pairings[j][3]:
                        rand_win = random.choice([1, 2])
                        print(
                            f"Player {shot_pairings[i][1]} and Player {shot_pairings[i][2]} both shot button {shot_pairings[i][3]}, Player {shot_pairings[i][rand_win]} gets the point!"
                        )
                        wins.append(shot_pairings[i][rand_win])
                        losses.append(shot_pairings[i][1 + rand_win % 2])
                        points[shot_pairings[i][rand_win]] += 1
                        points[shot_pairings[i][1 + rand_win % 2]] -= 1

                    elif (
                        shot_pairings[i][3] == shot_pairings[j][3] + 1
                        or shot_pairings[i][3] + 3 == shot_pairings[j][3] + 1
                    ):
                        print(
                            f"Player {shot_pairings[i][2]} beat Player {shot_pairings[i][1]}!"
                        )
                        wins.append(shot_pairings[i][2])
                        losses.append(shot_pairings[i][1])
                        points[shot_pairings[i][2]] += 1
                        points[shot_pairings[i][1]] -= 1
                    else:
                        print(
                            f"Player {shot_pairings[i][1]} beat Player {shot_pairings[i][2]}!"
                        )
                        wins.append(shot_pairings[i][1])
                        losses.append(shot_pairings[i][2])
                        points[shot_pairings[i][1]] += 1
                        points[shot_pairings[i][2]] -= 1

                    handled_pairings.add(i)
                    handled_pairings.add(j)

        for i in range(len(shot_pairings)):
            if i not in handled_pairings and shot_pairings[i][0] < time.time() - 1:
                print(
                    f"Player {shot_pairings[i][2]} beat Player {shot_pairings[i][1]}!"
                )
                wins.append(shot_pairings[i][2])
                losses.append(shot_pairings[i][1])
                points[shot_pairings[i][2]] += 1
                points[shot_pairings[i][1]] -= 1

                handled_pairings.add(i)

        shot_pairings = [
            shot_pairings[i]
            for i in range(len(shot_pairings))
            if i not in handled_pairings
        ]

        for w in wins:
            send(w, Win_str, int.to_bytes(w, 1, "big").decode("utf-8"))
        for l in losses:
            send(l, Loss_str, int.to_bytes(l, 1, "big").decode("utf-8"))

        # send any messages that need to be resent
        for i in range(device_count):
            for j in range(len(waiting_to_recv_ack[i])):
                if time.time() - waiting_to_recv_ack[i][j][1] > 1:
                    waiting_to_recv_ack[i][j][1] = time.time()
                    send(
                        i,
                        waiting_to_recv_ack[i][j][2],  # msg_type
                        waiting_to_recv_ack[i][j][3],  # msg_str
                        waiting_to_recv_ack[i][j][0],  # msg_id
                    )


if __name__ == "__main__":
    bt.run(main)

#include "BT.h"
#include <Board.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <timers.h>

// #define BT_test

#define UART_SPEED 9600 // baud rate
#define BUFFER_SIZE 128
#define RECV_TIMEOUT 50 // timeout in ms

#define HEAD 0xCC // head is the start of the packet, 0xCC marks the start of the packet
#define TAIL 0xB9 // tail is the end of the packet, 0xB9 marks the end of the packet
#define END1 0x0D // end1 and end2 are the end of the packet, 0x0D and 0x0A mark the end of the packet
#define END2 0x0A

typedef struct packet_struct
{
    uint8_t id;              // packet id
    uint8_t data[DATA_SIZE]; // packet data
    uint8_t length;          // length of the packet data
    uint8_t checksum;        // checksum of the packet
} *packet_t;

typedef struct buffer_struct
{
    packet_t buffer[BUFFER_SIZE]; // buffer for storing packets
    int head;
    int tail;
    int full;
    int empty; // flag on whether or not the buffer is empty
} *buffer_t;

// initialize structs
buffer_t buffer;
packet_t tmp_packet;

uint8_t in_packet_id = 0;     // packet id for incoming packets
uint8_t out_packet_id = 0;    // packet id for outgoing packets
uint8_t rx_buffer[DATA_SIZE]; // buffer for storing received data

int new_data = 0;
int packet_ready = 0;
uint8_t tmp_len = 0;

uint32_t last_recv_time = 0; // uint32_t because it is used with TIMERS_GetMilliSeconds(), which returns a uint32_t

void packet_parser(packet_t packet, uint8_t c);             // function to parse incoming packets
int send_packet(uint8_t id, uint8_t length, uint8_t *data); // function to send packets

int BT_Init()
{
    TIMER_Init();
    Uart1_Init(9600);
    buffer = (buffer_t)malloc(sizeof(struct buffer_struct));
    buffer->head = 0;
    buffer->tail = 0;
    buffer->full = 0;
    buffer->empty = 1;

    for (int i = 0; i < BUFFER_SIZE; i++)
    // allocate memory for each packet in the buffer
    {
        buffer->buffer[i] = (packet_t)malloc(sizeof(struct packet_struct));
    }

    tmp_packet = (packet_t)malloc(sizeof(struct packet_struct));

    return SUCCESS;
}

int BT_Recv(uint8_t *data)
{
    // if there is new data available and the time since the last received data is greater than the timeout
    if ((TIMERS_GetMilliSeconds() - last_recv_time) >= RECV_TIMEOUT && Uart1_rx(rx_buffer, DATA_SIZE) == SUCCESS)
    {
        new_data = 1; // set new_data flag to 1
        last_recv_time = TIMERS_GetMilliSeconds();
    }

    if (new_data) // if there is new data available
    {
        new_data = 0;   // set new_data flag to 0
        int start = -1; // start index of the packet
        for (int i = 0; i < DATA_SIZE; i++)
        // find the start of the packet
        {
            if (rx_buffer[i] == HEAD)
            // if the start of the packet is found
            {
                start = i; // set start to the index of the start of the packet
                break;
            }
        }

        if (start > -1)
        {
            for (int i = 0; i < DATA_SIZE; i++)
            // parse the packet
            {
                packet_parser(tmp_packet, rx_buffer[(start + i) % DATA_SIZE]);
                if (packet_ready && (tmp_packet->id > in_packet_id || tmp_packet->id == 0))
                // if the packet is ready and the packet id is greater than the current packet id
                {
                    packet_ready = 0;
                    in_packet_id = tmp_packet->id;                             // set the current packet id to the packet id
                    buffer->buffer[buffer->tail]->id = tmp_packet->id;         // set the buffer packet id to the packet id
                    buffer->buffer[buffer->tail]->length = tmp_packet->length; // set the buffer packet length to the packet length
                    for (int j = 0; j < tmp_packet->length; j++)
                    // copy the packet data to the buffer
                    {
                        buffer->buffer[buffer->tail]->data[j] = tmp_packet->data[j]; // copy the packet data to the buffer
                    }
                    buffer->tail = (buffer->tail + 1) % BUFFER_SIZE; // increment the buffer tail
                    if (buffer->tail == buffer->head)
                    // if the buffer tail is equal to the buffer head
                    {
                        buffer->full = 1; // set the buffer full flag to 1
                    }
                    buffer->empty = 0; // set the buffer empty flag to 0
                }
            }
        }
        memset(rx_buffer, 0, DATA_SIZE); // clear the rx buffer
    }

    if (!buffer->empty)
    {
        for (int i = 0; i < buffer->buffer[buffer->head]->length; i++)
        // copy the buffer data to the data buffer
        {
            data[i] = buffer->buffer[buffer->head]->data[i];
        }

        tmp_len = buffer->buffer[buffer->head]->length; // set the length of the data buffer

        buffer->head = (buffer->head + 1) % BUFFER_SIZE; // increment the buffer head
        if (buffer->head == buffer->tail)
        // if the buffer head is equal to the buffer tail
        {
            buffer->empty = 1;
        }
        buffer->full = 0;
        return tmp_len;
    }
    return ERROR;
}

// send data
int BT_Send(uint8_t *data, int len)
{
    int ret = send_packet(out_packet_id, len, data); // send the packet
    out_packet_id = (out_packet_id + 1) % 256;       // increment the packet id
    // must increment packet id to avoid duplicate packets
    return ret;
}

/**
 * each packet follows the format:
 * HEAD | LENGTH | ID | DATA | TAIL | CHECKSUM | END
 * where HEAD and TAIL are constant, LENGTH is the length of the data (not including the ID),
 * ID is the packet ID, DATA is the packet data/payload, and CHECKSUM is the xor of all the
 * bytes in the id & packet
 * */

void packet_parser(packet_t packet, uint8_t c)
{
    static uint8_t state = 0;
    static int index = 0;
    switch (state)
    {
    case 0:
        if (c == HEAD) // if the start of the packet is found
        {
            state = 1;
        }
        index = 0; // reset the index
        break;
    case 1:
        packet->length = c; // set the packet length to the length of the packet
        state = 2;          // set the state to 2
        break;
    case 2:
        packet->id = c;          // set the packet id to the id of the packet
        packet->checksum = c;    // set the packet checksum to the id of the packet
        if (packet->length == 0) // if the packet length is 0
        {
            state = 4;
        }
        else
        {
            state = 3;
            index = 0;
        }
        break;
    case 3:
        packet->data[index] = c; // set the packet data to the data of the packet
        packet->checksum ^= c;   // xor the packet checksum with the data of the packet
        index++;
        if (index == packet->length) // if the index is equal to the length of the packet
        {
            state = 4;
        }
        break;
    case 4:
        if (c == TAIL) // if the end of the packet is found
        {
            state = 5;
        }
        else
        {
            state = 0;
        }
        break;
    case 5:
        if (c == packet->checksum) // if the checksum of the packet is found
        {
            state = 6;
        }
        else
        {
            state = 0;
        }
        break;
    case 6:
        if (c == END1) // if the end of the packet is found
        {
            state = 7;
        }
        else
        {
            state = 0;
        }
        break;
    case 7:
        if (c == END2) // if the end of the packet is found
        {
            packet_ready = 1; // set the packet ready flag to 1
            state = 0;
        }
        else
        {
            state = 0;
        }
        break;
    default:
        break;
    }
}

uint8_t tx_tmp[DATA_SIZE + 7]; // temporary buffer for sending data, + 7 for the packet format

// send packet
int send_packet(uint8_t id, uint8_t length, uint8_t *data)
{
    uint8_t checksum = 0;
    tx_tmp[0] = HEAD;
    tx_tmp[1] = length;
    tx_tmp[2] = id;
    checksum = id;
    for (int i = 0; i < length; i++)
    // copy the data to the tx buffer
    {
        tx_tmp[i + 3] = data[i];
        checksum ^= data[i];
    }
    tx_tmp[length + 3] = TAIL;     // set the end of the packet
    tx_tmp[length + 4] = checksum; // set the checksum of the packet
    tx_tmp[length + 5] = END1;     // set the end of the packet
    tx_tmp[length + 6] = END2;     // set the end of the packet
    if (Uart1_tx(tx_tmp, length + 7) == ERROR)
    // if there is an error sending the packet
    {
        return ERROR;
    }
    return SUCCESS;
}

#ifdef BT_test
int main(void)
{
    BOARD_Init();
    BT_Init();
    printf("init done\n");

    uint8_t data[DATA_SIZE];

    while (1)
    {
        int len = BT_Recv(data);
        if (len != ERROR)
        {
            BT_Send(data, len);
        }
    }
}
#endif
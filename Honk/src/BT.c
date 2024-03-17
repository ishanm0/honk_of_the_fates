#include "BT.h"
#include <Board.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

// #define BT_test

#define UART_SPEED 9600
#define BUFFER_SIZE 128

#define HEAD 0xCC
#define TAIL 0xB9
#define END1 0x0D
#define END2 0x0A

typedef struct packet_struct
{
    uint8_t id;
    uint8_t data[DATA_SIZE];
    uint8_t length;
    uint8_t checksum;
} *packet_t;

typedef struct buffer_struct
{
    packet_t buffer[BUFFER_SIZE];
    int head;
    int tail;
    int full;
    int empty;
} *buffer_t;

buffer_t buffer;
packet_t tmp_packet;

uint8_t in_packet_id = 0;
uint8_t out_packet_id = 0;
uint8_t rx_buffer[DATA_SIZE];
int packet_ready = 0;
uint8_t tmp_len = 0;

void packet_parser(packet_t packet, uint8_t c);
int send_packet(uint8_t id, uint8_t length, uint8_t *data);

int BT_Init()
{
    Uart1_Init(9600);
    buffer = (buffer_t)malloc(sizeof(struct buffer_struct));
    buffer->head = 0;
    buffer->tail = 0;
    buffer->full = 0;
    buffer->empty = 1;

    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        buffer->buffer[i] = (packet_t)malloc(sizeof(struct packet_struct));
    }

    tmp_packet = (packet_t)malloc(sizeof(struct packet_struct));

    return SUCCESS;
}

int BT_Recv(uint8_t *data)
{
    memset(rx_buffer, 0, DATA_SIZE);
    if (Uart1_rx(rx_buffer, DATA_SIZE) == SUCCESS)
    {
        HAL_Delay(50);

        int start = 0;
        for (int i = 0; i < DATA_SIZE; i++)
        {
            if (rx_buffer[i] == HEAD)
            {
                start = i;
                break;
            }
            else if (i == DATA_SIZE - 1)
            {
                return ERROR;
            }
        }

        for (int i = 0; i < DATA_SIZE; i++)
        {
            packet_parser(tmp_packet, rx_buffer[(start + i) % DATA_SIZE]);
            if (packet_ready && (tmp_packet->id > in_packet_id || tmp_packet->id == 0))
            {
                packet_ready = 0;
                in_packet_id = tmp_packet->id;
                buffer->buffer[buffer->tail]->id = tmp_packet->id;
                buffer->buffer[buffer->tail]->length = tmp_packet->length;
                for (int j = 0; j < tmp_packet->length; j++)
                {
                    buffer->buffer[buffer->tail]->data[j] = tmp_packet->data[j];
                }
                buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
                if (buffer->tail == buffer->head)
                {
                    buffer->full = 1;
                }
                buffer->empty = 0;
            }
        }
    }

    if (!buffer->empty)
    {
        // printf("data: '");
        for (int i = 0; i < buffer->buffer[buffer->head]->length; i++)
        {
            // printf("%c", buffer->buffer[buffer->head]->data[i]);
            data[i] = buffer->buffer[buffer->head]->data[i];
        }
        // printf("'\n");

        // send_packet(buffer->buffer[buffer->head]->id, buffer->buffer[buffer->head]->length, buffer->buffer[buffer->head]->data);
        tmp_len = buffer->buffer[buffer->head]->length;

        buffer->head = (buffer->head + 1) % BUFFER_SIZE;
        if (buffer->head == buffer->tail)
        {
            buffer->empty = 1;
        }
        buffer->full = 0;
        return tmp_len;
    }
    return ERROR;
}

int BT_Send(uint8_t *data, int len)
{
    send_packet(out_packet_id, len, data);
    out_packet_id = (out_packet_id + 1) % 256;
    return SUCCESS;
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
        if (c == HEAD)
        {
            state = 1;
        }
        index = 0;
        break;
    case 1:
        packet->length = c;
        state = 2;
        break;
    case 2:
        packet->id = c;
        packet->checksum = c;
        if (packet->length == 0)
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
        packet->data[index] = c;
        packet->checksum ^= c;
        index++;
        if (index == packet->length)
        {
            state = 4;
        }
        break;
    case 4:
        if (c == TAIL)
        {
            state = 5;
        }
        else
        {
            state = 0;
        }
        break;
    case 5:
        if (c == packet->checksum)
        {
            state = 6;
        }
        else
        {
            state = 0;
        }
        break;
    case 6:
        if (c == END1)
        {
            state = 7;
        }
        else
        {
            state = 0;
        }
        break;
    case 7:
        if (c == END2)
        {
            packet_ready = 1;
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

uint8_t tx_tmp[DATA_SIZE + 7];

int send_packet(uint8_t id, uint8_t length, uint8_t *data)
{
    uint8_t checksum = 0;
    tx_tmp[0] = HEAD;
    tx_tmp[1] = length;
    tx_tmp[2] = id;
    checksum = id;
    for (int i = 0; i < length; i++)
    {
        tx_tmp[i + 3] = data[i];
        checksum ^= data[i];
    }
    tx_tmp[length + 3] = TAIL;
    tx_tmp[length + 4] = checksum;
    tx_tmp[length + 5] = END1;
    tx_tmp[length + 6] = END2;
    if (Uart1_tx(tx_tmp, length + 7) == ERROR)
    {
        return ERROR;
    }
    HAL_Delay(5);
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
            printf("data: '");
            for (int i = 0; i < len; i++)
            {
                printf("%c", data[i]);
            }
            printf("'\n");
            BT_Send(data, len);
        }
    }
}
#endif
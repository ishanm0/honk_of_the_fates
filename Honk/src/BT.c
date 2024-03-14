#include "BT.h"
#include <Board.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define UART_SPEED 9600
#define BUFFER_SIZE 128
#define DATA_SIZE 128

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

uint8_t packet_id = 0;

void init()
{
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
}

// each packet follows the format:
// HEAD | LENGTH | ID | DATA | TAIL | CHECKSUM | END
// where HEAD and TAIL are constant, LENGTH is the length of the data (not including the ID), ID is the packet ID, DATA is the packet data/payload, and CHECKSUM is the xor of all the bytes in the packet

int packet_ready = 0;

void packet_parser(packet_t packet, uint8_t c)
{
    static uint8_t state = 0;
    static int index = 0;
    // printf("%x %c\n", c, c);
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
        // printf("%d\n", packet->length);
        state = 2;
        break;
    case 2:
        packet->id = c;
        // printf("%d\n", packet->id);
        packet->checksum = c;
        if (packet->length == 0)
        {
            state = 4;
            // printf("data4\n");
        }
        else
        {
            state = 3;
            // printf("data3\n");
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

int main(void)
{
    BOARD_Init();
    Uart1_Init(9600);
    init(&buffer, &tmp_packet);
    uint8_t data[50];
    // uint8_t last_data[1];
    // uint8_t last_data[DATA_SIZE];
    printf("init done\n");
    while (1)
    {
        // take in one byte at a time
        if (Uart1_rx(data, 50) == SUCCESS) {
            // feed into packet parser
            for (int i = 0; i < 50; i++) {
                // if (data[i] != last_data[i]) {
                    // if (data[i] != 0xff) {
                    // compare data[i] to dollar sign
                    if (data[i] != 0xff) {
                        printf("data: %c\n", data[i]);
                    } //else {
                    //     continue;
                    // }
                    // last_data[i] = data[i];
                
                // printf("data: %c %x\n", data[i], data[i]);
            }
            HAL_Delay(50);
            continue;
            packet_parser(tmp_packet, data[0]);
            // if packet is ready, copy to buffer
            if (packet_ready)
            {
                packet_ready = 0;
                // copy packet to buffer
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
            while (!buffer->empty) {
                printf("buffer not empty\n");
                printf("id/packet: %d/%x\n", buffer->buffer[buffer->head]->id, packet_id);
                for (int i = 0; i < buffer->buffer[buffer->head]->length; i++)
                {
                    printf("%c", buffer->buffer[buffer->head]->data[i]);
                }
                printf("\n");
                buffer->head = (buffer->head + 1) % BUFFER_SIZE;
                if (buffer->head == buffer->tail)
                {
                    buffer->empty = 1;
                }
                buffer->full = 0;
            }
        }



        // if (Uart1_rx(data, DATA_SIZE) == 1)
        // {
        //     // int start = 0;
        //     // for (int i = 0; i < DATA_SIZE; i++)
        //     // {
        //     //     if (data[i] != last_data[i])
        //     //     {
        //     //         start = i;
        //     //         break;
        //     //     }
        //     // }
        //     // uint8_t c;

        //     int start = 0;
        //     for (int i = 0; i < DATA_SIZE; i++)
        //     {
        //         if (data[i] == HEAD)
        //         {
        //             start = 0;
        //             break;
        //         }
        //     }

        //     for (int i = 0; i < DATA_SIZE; i++)
        //     {
        //         // c = data[i];
        //         packet_parser(tmp_packet, data[(start + i) % DATA_SIZE]);
        //         // printf("data[%d]: %x\n", i, data[i]);
        //         // packet_parser(tmp_packet, data[i]);
        //         if (packet_ready)
        //         {
        //             packet_ready = 0;
        //             // printf("packet ready\n");
        //             // copy packet to buffer
        //             buffer->buffer[buffer->tail]->id = tmp_packet->id;
        //             buffer->buffer[buffer->tail]->length = tmp_packet->length;
        //             for (int j = 0; j < tmp_packet->length; j++)
        //             {
        //                 buffer->buffer[buffer->tail]->data[j] = tmp_packet->data[j];
        //             }
        //             buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
        //             if (buffer->tail == buffer->head)
        //             {
        //                 buffer->full = 1;
        //             }
        //             buffer->empty = 0;
        //         }
        //     }
        // }

        // while (!buffer->empty)
        // {
        //     // if (buffer->buffer[buffer->head]->data[0] > packet_id)
        //     // {
        //         // packet_id = buffer->buffer[buffer->head]->data[0];
        //         printf("buffer not empty\n");
        //         printf("id/packet: %d/%x\n", buffer->buffer[buffer->head]->id, packet_id);
        //         // printf("length: %d\n", buffer->buffer[buffer->head]->length);
        //         for (int i = 0; i < buffer->buffer[buffer->head]->length; i++)
        //         {
        //             printf("%c", buffer->buffer[buffer->head]->data[i]);
        //             // printf("data[%d]: %d\n", i, buffer->buffer[buffer->head]->data[i]);
        //         }
        //         printf("\n");
        //     // } else {
        //     //     printf("packet id: %x\n", buffer->buffer[buffer->head]->data[0]);
        //     // }
        //     buffer->head = (buffer->head + 1) % BUFFER_SIZE;
        //     if (buffer->head == buffer->tail)
        //     {
        //         buffer->empty = 1;
        //     }
        //     buffer->full = 0;
        // }

        // Uart1_rx(data, 128);
        HAL_Delay(100);
    }
    // uint8_t data[128];
    // memset(data, 0, 128);
    // while (1)
    // {
    //     // memset(data, 0, 128);
    //     Uart1_rx(data, 128);                  // read data from bluetooth
    //     printf("data: %s\n", data);           // print data to console
    //     Uart1_tx(data, strlen((char *)data)); // send data to bluetooth
    //     HAL_Delay(1000);
    //     // if uart successfully reads data, clear the buffer
    //     // this should not work, and should cause
    //     Uart1_rx(data, 128);
    //     for (int i = 0; i < 128; i++)
    //     {
    //         data[i] = 0;
    //     }
    // }
}
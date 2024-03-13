#include "BT.h"
#include <Board.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define UART_SPEED 9600

#define HEAD 0xCC
#define TAIL 0xB9
#define END 0x0D0A

typedef struct packet_struct {
    uint8_t id;
    uint8_t data[128];
    uint8_t length;
    uint8_t checksum;
} *packet_t;

// each packet follows the format:
// HEAD | LENGTH | ID | DATA | TAIL | CHECKSUM | END
// where HEAD and TAIL are constant, LENGTH is the length of the data (not including the ID), ID is the packet ID, DATA is the packet data/payload, and CHECKSUM is the xor of all the bytes in the packet

void packet_parser(packet_t *packet, uint8_t c)
{
    static uint8_t state = 0;
    
}

int main(void)
{
    BOARD_Init();
    Uart1_Init(9600);
    uint8_t data[128];
    memset(data, 0, 128);
    while (1)
    {
        Uart1_rx(data, 128);                  // read data from bluetooth
        printf("data: %s\n", data);           // print data to console
        Uart1_tx(data, strlen((char *)data)); // send data to bluetooth
        HAL_Delay(1000);
        // if uart successfully reads data, clear the buffer
        // this should not work, and should cause
        Uart1_rx(data, 128);
    }
}
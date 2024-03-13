#include "BT.h"
#include <Board.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define UART_SPEED 9600

int main(void)
{
    BOARD_Init();
    Uart1_Init(9600);
    uint8_t data[128];
    uint8_t data2[128];
    strcpy((char *)data2, "please no more\n");
    while (1)
    {
        Uart1_rx(data, 128);
        printf("data: %s\n", data);
        Uart1_tx(data, strlen((char *)data));
        HAL_Delay(1000);
    }
}
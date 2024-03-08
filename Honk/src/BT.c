#include "BT.h"
#include <Board.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    BOARD_Init();
    Uart1_Init(9600);
    // USART1->CR3 |= USART_CR3_CTSE; // Enable CTS
    // USART1->CR3 |= USART_CR3_RTSE; // Enable RTS
    uint8_t data[128];
    uint8_t data2[128];
    strcpy((char *)data2, "help\n");
    while (1)
    {
        // Uart1_rx(data, 128);
        // Uart1_tx(data2, strlen((char *)data2))
        // printf("%s\n", data);
        // Uart1_tx(data, strlen((char *)data));
        // send data to BT via uart
        // Uart1_tx(data2, strlen((char *)data2));
        // HAL_Delay(1000);
        Uart1_rx(data, 128);
        printf("received: %s\n", data);
        HAL_Delay(1000);
        Uart1_tx(data2, strlen((char *)data2));
        HAL_Delay(1000);
    }
}
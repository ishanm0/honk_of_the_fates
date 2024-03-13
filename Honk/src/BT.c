#include "BT.h"
#include <Board.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define UART_SPEED 9600

// scan for bluetooth devices
void BT_scan(void) {
    // send AT command to BT
    // wait for response
    // print response
    char rx_data[100];
    int8_t connected = FALSE;

    while (!connected) {
        memset(rx_data, 0, sizeof(rx_data));

        // send AT command to start scanning
        uint8_t cmd[] = "AT+INQ\r\n";
        Uart1_tx(cmd, strlen((char *)cmd));

        // wait for response from BT
        HAL_Delay(1000); // wait for response

        //
    }
}


int main(void)
{
    BOARD_Init();
    Uart1_Init(9600);
    // USART1->CR3 |= USART_CR3_CTSE; // Enable CTS
    // USART1->CR3 |= USART_CR3_RTSE; // Enable RTS
    uint8_t data[128];
    uint8_t data2[128];
    strcpy((char *)data2, "please no more\n");
    // 
    while (1)
    {
        BT_scan();
        // Uart1_rx(data, 128);
        // Uart1_tx(data2, strlen((char *)data2))
        // printf("%s\n", data);
        // Uart1_tx(data, strlen((char *)data));
        // send data to BT via uart
        // Uart1_tx(data2, strlen((char *)data2));
        // HAL_Delay(1000);
        // Uart1_rx(data, 128);
        // printf("received: %s\n", data);
        // HAL_Delay(1000);
        // Uart1_tx(data2, strlen((char *)data2));
        // HAL_Delay(1000);
    }
}
// // #include <WS2812.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <Board.h>
// #include <ADC.h>
// #include <pwm.h>
// #include <math.h>

// #include <string.h>
// #include <stm32f4xx_hal_spi.h>
// #include "stm32f4xx.h"

// #include <inttypes.h>

// // Set up SPI:

// #define WS2812_SPI SPI2
// #define WS2812_SPI_GPIO_PORT GPIOB
// #define WS2812_SPI_SCK_PIN GPIO_PIN_13  // PB13 for SPI2_SCK
// #define WS2812_SPI_MOSI_PIN GPIO_PIN_15 // PB15 for SPI2_MOSI

// SPI_HandleTypeDef hspi;
// /* WS2812 SPI initialization function */
// void WS2812_SPI_Init(void)
// {

//     /* SPI peripheral clock enable */
//     __HAL_RCC_SPI2_CLK_ENABLE();
//     __HAL_RCC_GPIOB_CLK_ENABLE();

//     /* SPI SCK and MOSI GPIO pin configuration */
//     GPIO_InitTypeDef GPIO_InitStruct;
//     GPIO_InitStruct.Pin = WS2812_SPI_SCK_PIN | WS2812_SPI_MOSI_PIN;
//     GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//     GPIO_InitStruct.Pull = GPIO_PULLUP;
//     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//     GPIO_InitStruct.Alternate = GPIO_AF5_SPI2; // Alternate function 5 for SPI2
//     HAL_GPIO_Init(WS2812_SPI_GPIO_PORT, &GPIO_InitStruct);

//     /* SPI initialization */
//     hspi.Instance = WS2812_SPI;
//     hspi.Init.Mode = SPI_MODE_MASTER;
//     hspi.Init.Direction = SPI_DIRECTION_1LINE;
//     hspi.Init.DataSize = SPI_DATASIZE_8BIT;
//     hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
//     hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
//     hspi.Init.NSS = SPI_NSS_SOFT;
//     hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // Adjust this as necessary for your LED timing
//     hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
//     hspi.Init.TIMode = SPI_TIMODE_DISABLE;
//     hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
//     hspi.Init.CRCPolynomial = 10039; // previously 10
//     HAL_SPI_Init(&hspi);
// }

// // /* Function to send data to WS2812 LEDs */
// // void WS2812_SendData(uint8_t *data, uint16_t len)
// // {
// //     HAL_SPI_Transmit(&WS2812_SPI, data, len, HAL_MAX_DELAY);
// // }

// // SELF WRITTEN:

// // // RGB:


// #define NUM_LED 120
// uint8_t LED_Data[NUM_LED][4];

// // extern SPI_HandleTypeDef hspi1;

// #define USE_BRIGHTNESS 1
// int brightness = 20;

// void setLED(int led, int RED, int GREEN, int BLUE)
// {
//     LED_Data[led][0] = led;
//     LED_Data[led][1] = GREEN;
//     LED_Data[led][2] = RED;
//     LED_Data[led][3] = BLUE;
// }

// void ws2812_spi(int GREEN, int RED, int BLUE)
// {
// #if USE_BRIGHTNESS
//     if (brightness > 100)
//         brightness = 100;
//     GREEN = GREEN * brightness / 100;
//     RED = RED * brightness / 100;
//     BLUE = BLUE * brightness / 100;
// #endif
//     uint32_t color = GREEN << 16 | RED << 8 | BLUE;
//     // printBinary(color);
//     uint8_t sendData[24];
//     int indx = 0;

//     for (int i = 23; i >= 0; i--)
//     {
//         if (((color >> i) & 0x01) == 1)
//             sendData[indx++] = 0b110; // store 1
//         else
//             sendData[indx++] = 0b100; // store 0
//     }

//     HAL_SPI_Transmit(&hspi, sendData, 24, 1000);
// }

// void WS2812_Send(void)
// {
//     for (int i = 0; i < NUM_LED; i++)
//     {
//         ws2812_spi(LED_Data[i][1], LED_Data[i][2], LED_Data[i][3]);
//     }
//     HAL_Delay(1);
// }

// // MAIN:

// int R, G, B, C, X, H_prime;
// char flag;
// int initFunc(void) // initializes everything we need
// {
//     BOARD_Init();
//     // PWM_Init();
//     // TIMER_Init();

//     // initialize SPI

//     // RGB library
//     WS2812_SPI_Init();
//     //   ws2812_init();
//     //   ws2812_pixel_all(255, 0, 255);

//     return SUCCESS;
// }

// int main(void)
// {
//     initFunc();

//     //   int state = 0;

//     int minled = 0;
//     int maxled = 10;
//     while (TRUE)
//     {
//         minled = (minled + 1) % NUM_LED;
//         maxled = (maxled + 1) % NUM_LED;
//         for (int i = 0; i < NUM_LED; i++)
//         {
//             if ((i >= minled && i < maxled) || (minled > maxled && (i >= minled || i < maxled))){
//                 setLED(i, 0, 255, 0);
//             } else {
//                 setLED(i, 0, 0, 0);
//                 // setLED(i, 255, 0, 0);
//             }
                
//         }
//         printf("help\n");
//         WS2812_Send();
//         HAL_Delay(100);

//         // for (int i = minled; i < maxled; i++)
//         // {
//         //     setLED(i, 0, 255, 0);
//         // }
//         // WS2812_Send();
//         // HAL_Delay(1000);

//         // for (int i = minled; i < maxled; i++)
//         // {
//         //     setLED(i, 0, 0, 255);
//         // }
//         // WS2812_Send();
//         // HAL_Delay(1000);
//     }
// }
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

// // GPT assisted:

// #define WS2812_SPI SPI2
// #define WS2812_SPI_GPIO_PORT GPIOB
// #define WS2812_SPI_SCK_PIN GPIO_PIN_13  // PB13 for SPI2_SCK
// #define WS2812_SPI_MOSI_PIN GPIO_PIN_15 // PB15 for SPI2_MOSI

// SPI_HandleTypeDef hspi;
// /* WS2812 SPI initialization function */
// void WS2812_SPI_Init(void)
// {

//   /* SPI peripheral clock enable */
//   __HAL_RCC_SPI2_CLK_ENABLE();
//   __HAL_RCC_GPIOB_CLK_ENABLE();

//   /* SPI SCK and MOSI GPIO pin configuration */
//   GPIO_InitTypeDef GPIO_InitStruct;
//   GPIO_InitStruct.Pin = WS2812_SPI_SCK_PIN | WS2812_SPI_MOSI_PIN;
//   GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//   GPIO_InitStruct.Pull = GPIO_PULLUP;
//   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//   GPIO_InitStruct.Alternate = GPIO_AF5_SPI2; // Alternate function 5 for SPI2
//   HAL_GPIO_Init(WS2812_SPI_GPIO_PORT, &GPIO_InitStruct);

//   /* SPI initialization */
//   hspi.Instance = WS2812_SPI;
//   hspi.Init.Mode = SPI_MODE_MASTER;
//   hspi.Init.Direction = SPI_DIRECTION_1LINE;
//   hspi.Init.DataSize = SPI_DATASIZE_8BIT;
//   hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
//   hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
//   hspi.Init.NSS = SPI_NSS_SOFT;
//   hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // Adjust this as necessary for your LED timing
//   hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
//   hspi.Init.TIMode = SPI_TIMODE_DISABLE;
//   hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
//   hspi.Init.CRCPolynomial = 10;
//   HAL_SPI_Init(&hspi);
// }

// // /* Function to send data to WS2812 LEDs */
// // void WS2812_SendData(uint8_t *data, uint16_t len)
// // {
// //     HAL_SPI_Transmit(&WS2812_SPI, data, len, HAL_MAX_DELAY);
// // }

// // SELF WRITTEN:

// // RGB:
// #define WS2812_NUM_LEDS 8
// // #define WS2812_SPI_HANDLE hspi

// #define WS2812_RESET_PULSE 10
// #define WS2812_BUFFER_SIZE (WS2812_NUM_LEDS * 24 + WS2812_RESET_PULSE)

// // extern SPI_HandleTypeDef WS2812_SPI_HANDLE;
// // extern uint8_t ws2812_buffer[];

// uint8_t ws2812_buffer[WS2812_BUFFER_SIZE];

// void ws2812_send_spi(void)
// {
//   // HAL_SPI_Transmit(&WS2812_SPI_HANDLE, ws2812_buffer, WS2812_BUFFER_SIZE, HAL_MAX_DELAY);
//   HAL_SPI_Transmit(&hspi, ws2812_buffer, WS2812_BUFFER_SIZE, HAL_MAX_DELAY);
//   // HAL_SPI_Transmit(&WS2812_SPI_HANDLE, ws2812_buffer, WS2812_BUFFER_SIZE, 0xFFFFFFFFU);
// }

// void ws2812_init(void)
// {
//   memset(ws2812_buffer, 0, WS2812_BUFFER_SIZE);
//   ws2812_send_spi();
// }

// #define WS2812_FILL_BUFFER(COLOR)             \
//   for (uint8_t mask = 0x80; mask; mask >>= 1) \
//   {                                           \
//     if (COLOR & mask)                         \
//     {                                         \
//       *ptr++ = 0xfc;                          \
//     }                                         \
//     else                                      \
//     {                                         \
//       *ptr++ = 0x80;                          \
//     }                                         \
//   }

// void ws2812_pixel(uint16_t led_no, uint8_t r, uint8_t g, uint8_t b)
// {
//   uint8_t *ptr = &ws2812_buffer[24 * led_no];
//   WS2812_FILL_BUFFER(g);
//   WS2812_FILL_BUFFER(r);
//   WS2812_FILL_BUFFER(b);
// }

// void ws2812_pixel_all(uint8_t r, uint8_t g, uint8_t b)
// {
//   uint8_t *ptr = ws2812_buffer;
//   for (uint16_t i = 0; i < WS2812_NUM_LEDS; ++i)
//   {
//     printf("%x %x %x\n", r, g, b);
//     WS2812_FILL_BUFFER(g);
//     WS2812_FILL_BUFFER(r);
//     WS2812_FILL_BUFFER(b);
//   }
//   for (int i = 0; i < 64; i++){
//   printf("%x", ws2812_buffer[i]);
//   if ((i+1) % 8 == 0)
//   printf("\n");
//   }
// }

// // MAIN:

// int R, G, B, C, X, H_prime;
// char flag;
// int initFunc(void) // initializes everything we need
// {
//   BOARD_Init();
//   // PWM_Init();
//   // TIMER_Init();

//   // initialize SPI

//   // RGB library
//   WS2812_SPI_Init();
//   ws2812_init();
//   ws2812_pixel_all(255, 0, 255);

//   return SUCCESS;
// }

// int main(void)
// {
//   initFunc();

//   int state = 0;

//   while (TRUE)
//   {
//     HAL_Delay(1000);

//     ws2812_pixel_all(255, 0, 0);
//     // ws2812_pixel(state, 255, 0, 0);
//     state++;

//     // if(state == 0){
//     //   ws2812_pixel_all(255, 0, 0);
//     //   state = 1;
//     // } else if(state == 1){
//     //   ws2812_pixel_all(0, 0, 255);
//     //   state = 2;
//     // } else {
//     //   ws2812_pixel_all(0, 255, 0);
//     //   state = 0;
//     // }
//     printf("state: %d\n", state);
//     for (int i = 0; i < WS2812_BUFFER_SIZE; i++){
//       printf("%x", ws2812_buffer[i]);
//       if ((i+1) % 24 == 0)
//       printf("\n");
//     }
//     ws2812_send_spi();

//   }
// }
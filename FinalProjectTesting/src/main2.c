// #include <WS2812.h>
#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <ADC.h>
#include <pwm.h>
#include <math.h>

#include <string.h>
#include <stm32f4xx_hal_spi.h>
#include "stm32f4xx.h"

#include <inttypes.h>

void printBinary(uint32_t num)
{
    for (int i = 31; i >= 0; --i)
    {
        printf("%d", (num >> i) & 1);
        if (i % 4 == 0)
            printf(" ");
    }
    printf("\n");
}

// Set up SPI:

#define WS2812_SPI SPI2
#define WS2812_SPI_GPIO_PORT GPIOB
#define WS2812_SPI_SCK_PIN GPIO_PIN_13  // PB13 for SPI2_SCK
#define WS2812_SPI_MOSI_PIN GPIO_PIN_15 // PB15 for SPI2_MOSI

SPI_HandleTypeDef hspi;
/* WS2812 SPI initialization function */
void WS2812_SPI_Init(void)
{

    /* SPI peripheral clock enable */
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* SPI SCK and MOSI GPIO pin configuration */
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = WS2812_SPI_SCK_PIN | WS2812_SPI_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2; // Alternate function 5 for SPI2
    HAL_GPIO_Init(WS2812_SPI_GPIO_PORT, &GPIO_InitStruct);

    /* SPI initialization */
    hspi.Instance = WS2812_SPI;
    hspi.Init.Mode = SPI_MODE_MASTER;
    hspi.Init.Direction = SPI_DIRECTION_1LINE;
    hspi.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi.Init.NSS = SPI_NSS_SOFT;
    hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // Adjust this as necessary for your LED timing
    hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi.Init.CRCPolynomial = 10039; // previously 10
    HAL_SPI_Init(&hspi);
}

// /* Function to send data to WS2812 LEDs */
// void WS2812_SendData(uint8_t *data, uint16_t len)
// {
//     HAL_SPI_Transmit(&WS2812_SPI, data, len, HAL_MAX_DELAY);
// }

// SELF WRITTEN:

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

#define NUM_LED 120
uint8_t LED_Data[NUM_LED][4];

// extern SPI_HandleTypeDef hspi1;

#define USE_BRIGHTNESS 1
int brightness = 20;

void setLED(int led, int RED, int GREEN, int BLUE)
{
    LED_Data[led][0] = led;
    LED_Data[led][1] = GREEN;
    LED_Data[led][2] = RED;
    LED_Data[led][3] = BLUE;
}

void ws2812_spi(int GREEN, int RED, int BLUE)
{
#if USE_BRIGHTNESS
    if (brightness > 100)
        brightness = 100;
    GREEN = GREEN * brightness / 100;
    RED = RED * brightness / 100;
    BLUE = BLUE * brightness / 100;
#endif
    uint32_t color = GREEN << 16 | RED << 8 | BLUE;
    // printBinary(color);
    uint8_t sendData[24];
    int indx = 0;

    for (int i = 23; i >= 0; i--)
    {
        if (((color >> i) & 0x01) == 1)
            sendData[indx++] = 0b110; // store 1
        else
            sendData[indx++] = 0b100; // store 0
    }

    HAL_SPI_Transmit(&hspi, sendData, 24, 1000);
}

void WS2812_Send(void)
{
    for (int i = 0; i < NUM_LED; i++)
    {
        ws2812_spi(LED_Data[i][1], LED_Data[i][2], LED_Data[i][3]);
    }
    HAL_Delay(1);
}

// MAIN:

int R, G, B, C, X, H_prime;
char flag;
int initFunc(void) // initializes everything we need
{
    BOARD_Init();
    // PWM_Init();
    // TIMER_Init();

    // initialize SPI

    // RGB library
    WS2812_SPI_Init();
    //   ws2812_init();
    //   ws2812_pixel_all(255, 0, 255);

    return SUCCESS;
}

// int angleToColours(int inAngle)
// {
//     int mathAngle = inAngle;
//     // divisions are centered every 120 (0, 120, 240)
//     // subdivisions are every 120 offset 60 (60, 180, 300)
//     /// Red is min at 300(reverse wrapping) and 180, max at 60
//     if (inAngle > 300 || inAngle < 180)
//     {
//         // shift into easier math range 0 to 240 (add 60)
//         if (inAngle > 300)
//         {
//             mathAngle -= 300; // (wrapping with the +60)
//         }
//         else
//         {
//             mathAngle += 60;
//         }
//         // now that it is [0, 240]:
//         // if <120
//         if (mathAngle <= 120)
//         {
//             valueR = (255 * mathAngle) / 120;
//         }
//         else
//         {
//             // flip the value of mathAngle so I can use the 0 to 120 transfer equation.
//             mathAngle = 120 - (mathAngle - 120);
//             // now can proceed as normal.
//             valueR = (255 * mathAngle) / 120;
//         }
//     }
//     else
//     {
//         valueR = 0;
//     }

//     mathAngle = inAngle;
//     /// Green is min at 60 and 300, max at 180
//     if (inAngle > 60 && inAngle < 300)
//     {
//         // shift into easier math range 0 to 240 (subtract 60)
//         mathAngle -= 60;
//         // now that it is [0, 240]:
//         // if <120
//         if (mathAngle <= 120)
//         {
//             valueG = (255 * mathAngle) / 120;
//         }
//         else
//         {
//             // flip the value of mathAngle so I can use the 0 to 120 transfer equation.
//             mathAngle = 120 - (mathAngle - 120);
//             // now can proceed as normal.
//             valueG = (255 * mathAngle) / 120;
//         }
//     }
//     else
//     {
//         valueG = 0;
//     }

//     mathAngle = inAngle;
//     /// Blue is min at 180 and 60(wrapping), max at 300
//     if (inAngle > 180 || inAngle < 60)
//     {
//         // shift into easier math range 0 to 240 (add 180 ... I think)
//         if (inAngle > 180)
//         {
//             mathAngle -= 180; // (wrapping with the +60)
//         }
//         else
//         {
//             mathAngle += 180;
//         }
//         // now that it is [0, 240]:
//         // if <120
//         if (mathAngle <= 120)
//         {
//             valueB = (255 * mathAngle) / 120;
//         }
//         else
//         {
//             // flip the value of mathAngle so I can use the 0 to 120 transfer equation.
//             mathAngle = 120 - (mathAngle - 120);
//             // now can proceed as normal.
//             valueB = (255 * mathAngle) / 120;
//         }
//     }
//     else
//     {
//         valueB = 0;
//     }

//     return 0;
// }

int R, G, B;
void setColour(int inAngle)
{
    int H = 0;
    int X = 0;
    int C = 0;
    int m = 0;

    // int R = 0;
    // int G = 0;
    // int B = 0;

    int S = 100;
    int L = 50;
    C = ((100.0 - abs(2.0 * L - 100)) * S) * 256 / 100 / 100;
    H = abs(inAngle) * 120 / 60;
    X = (C * (100 - abs((H % 200) - 100))) * 256 / 100 / 100;
    m = (L - C / 2) * 255 / 100;
    if (H / 120 >= 5)
    {
        R = C + m;
        G = 0 + m;
        B = X + m;
    }
    else if (H / 120 >= 4)
    {
        R = X + m;
        G = 0 + m;
        B = C + m;
    }
    else if (H / 120 >= 3)
    {
        R = 0 + m;
        G = X + m;
        B = C + m;
    }
    else if (H / 120 >= 2)
    {
        R = 0 + m;
        G = C + m;
        B = X + m;
    }
    else if (H / 120 >= 1)
    {
        R = X + m;
        G = C + m;
        B = 0 + m;
    }
    else
    {
        R = C + m;
        G = X + m;
        B = 0 + m;
    }
}

int main(void)
{
    initFunc();

    //   int state = 0;

    int minled = 0;
    int maxled = 10;
    while (TRUE)
    {
        minled = (minled + 1) % NUM_LED;
        maxled = (maxled + 1) % NUM_LED;
        for (int i = 0; i < NUM_LED; i++)
        {
            if ((i >= minled && i < maxled) || (minled > maxled && (i >= minled || i < maxled))){
                setColour(i);
                setLED(i, R, G, B);
            } else {
                setLED(i, 0, 0, 0);
                // setLED(i, 255, 0, 0);
            }
                
        }
        WS2812_Send();
        HAL_Delay(1);

        // for (int i = minled; i < maxled; i++)
        // {
        //     setLED(i, 0, 255, 0);
        // }
        // WS2812_Send();
        // HAL_Delay(1000);

        // for (int i = minled; i < maxled; i++)
        // {
        //     setLED(i, 0, 0, 255);
        // }
        // WS2812_Send();
        // HAL_Delay(1000);
    }
}
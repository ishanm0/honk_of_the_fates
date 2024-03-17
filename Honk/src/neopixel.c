// #include <WS2812.h>
#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <ADC.h>
#include <pwm.h>
#include <math.h>
#include <timers.h>

#include <string.h>
#include <stm32f4xx_hal_spi.h>
#include "stm32f4xx.h"

#include <inttypes.h>

#include "neopixel.h"

// void printBinary(uint32_t num)
// {
//     for (int i = 31; i >= 0; --i)
//     {
//         printf("%d", (num >> i) & 1);
//         if (i % 4 == 0)
//             printf(" ");
//     }
//     printf("\n");
// }

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

// RGB functionality

// set up LED color storage
#define NUM_LED 23
uint8_t LED_Data[NUM_LED][4];
uint8_t DEFAULT_RGB[] = {255, 0, 0};

// brightness control on if 1, off if 0
#define USE_BRIGHTNESS 1

// values 0-100 (%), sets brightness of pixels.
//      Affects ws2812_spi()
int brightness = 20;

void setDefaultRGB(int RED, int GREEN, int BLUE)
{
    DEFAULT_RGB[0] = RED;
    DEFAULT_RGB[1] = GREEN;
    DEFAULT_RGB[2] = BLUE;
}

void getDefaultRGB(int *RED, int *GREEN, int *BLUE)
{
    *RED = DEFAULT_RGB[0];
    *GREEN = DEFAULT_RGB[1];
    *BLUE = DEFAULT_RGB[2];
}

// set a given LED to given RGB colour values
void setLED(int led, int RED, int GREEN, int BLUE)
{
    if (led < 0 || led >= NUM_LED)
        return;

    LED_Data[led][0] = led;
    LED_Data[led][1] = GREEN;
    LED_Data[led][2] = RED;
    LED_Data[led][3] = BLUE;
}

// sends the correct SPI message to correspond to given RGB colours
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

// sends an updated stream of RGB LED controll signals to each LED in the system.
void WS2812_Send(void)
{
    for (int i = 0; i < NUM_LED; i++)
    {
        ws2812_spi(LED_Data[i][1], LED_Data[i][2], LED_Data[i][3]);
    }
    HAL_Delay(1);
}

// pulser variables
int runningPulse = FALSE; // is s pulse animation active
int pulse_propagation = 0;

uint32_t lastUpdate = 0;
uint32_t tempTime = 0;

// When called will check for RGB LED animation status and
//  if the runningPulse flag is == TRUE it will
//  service a current animation or start a new one if there is none active
//  This function is compute light when called unless it si time for a new frame.
//      Thus it is safe to call repeatedly.
// the "start" input can start a new pulse if one hasn't been started yet
//      it should be either a bool TRUE or FALSE
void spellPulse(spell used_spell, int start)
{
    // check current time
    tempTime = TIMERS_GetMilliSeconds();

    // the "start" input can start a new pulse if one hasn't been started yet
    if (start)
        runningPulse = TRUE;

    // If currently running an animation and ready to do the next frame:
    if (runningPulse && tempTime > (lastUpdate + 10))
    {
        printf("Yes1: %lu : %lu\n", tempTime, lastUpdate);
        if (pulse_propagation >= NUM_LED)
        {
            runningPulse = FALSE;
            setLED(pulse_propagation, DEFAULT_RGB[0], DEFAULT_RGB[1], DEFAULT_RGB[2]);
            pulse_propagation = 0;
        }
        else
        {
            pulse_propagation++;
            switch (used_spell)
            {
            case UNSPECIFIED:
                // Turns the pulse pixel off.
                setLED(pulse_propagation - 1, DEFAULT_RGB[0], DEFAULT_RGB[1], DEFAULT_RGB[2]);
                setLED(pulse_propagation, 255, 255, 255);
                break;
            case ROCK:
                // Turns the pulse pixel off.
                setLED(pulse_propagation - 1, DEFAULT_RGB[0], DEFAULT_RGB[1], DEFAULT_RGB[2]);
                setLED(pulse_propagation, 0, 255, 0);
                break;
            case PAPER:
                // Turns the pulse pixel off.
                setLED(pulse_propagation - 1, DEFAULT_RGB[0], DEFAULT_RGB[1], DEFAULT_RGB[2]);
                setLED(pulse_propagation, 255, 255, 0);
                break;
            case SISSORS:
                // Turns the pulse pixel off.
                setLED(pulse_propagation - 1, DEFAULT_RGB[0], DEFAULT_RGB[1], DEFAULT_RGB[2]);
                setLED(pulse_propagation, 255, 0, 0);
                break;
            }
        }
        // update last update time store
        lastUpdate = TIMERS_GetMilliSeconds();
        WS2812_Send();
    }
}

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

// so you can do it without a set spell
// void spellPulse(void){
//     spellPulse(UNSPECIFIED);
// }

// MAIN:

// int R, G, B, C, X, H_prime;
// char flag;
// int initFunc(void) // initializes everything we need
// {
//     BOARD_Init();
//     // PWM_Init();
//     TIMER_Init();

//     // initialize SPI

//     // RGB library
//     WS2812_SPI_Init();

//     // everything should be initialized
//     return SUCCESS;
// }

// int main(void)
// {
//     initFunc();

//     HAL_Delay(100);
//     // initialize all LEDs to red
//     for (int i = 0; i < NUM_LED; i++) setLED(i, 0, 255, 0);
//     WS2812_Send();
//     HAL_Delay(5000);

//     runningPulse = TRUE;

//     while (TRUE)
//     {
//         // spell abcss = UNSPECIFIED;
//         spellPulse(UNSPECIFIED);
//         HAL_Delay(10);
//         if(!runningPulse){
//             runningPulse = TRUE;
//         }

//     }
// }

// minled = (minled + 1) % NUM_LED;
// maxled = (maxled + 1) % NUM_LED;
// for (int i = 0; i < NUM_LED; i++)
// {
//     if ((i >= 0 && i < NUM_LED) || (0 > NUM_LED && (i >= 0 || i < NUM_LED))){
//         setColour(i);
//         setLED(i, R, G, B);
//     } else {
//         setLED(i, 0, 0, 0);
//         // setLED(i, 255, 0, 0);
//     }

// }
// for (int i=0; i<4; i++)
// {
//     setLED(i, 255, 0, 0);
// }
// WS2812_Send();
// HAL_Delay(1000);

// for (int i=0; i<4; i++)
// {
//     setLED(i, 0, 255, 0);
// }
// WS2812_Send();
// HAL_Delay(1000);

// for (int i=0; i<4; i++)
// {
//     setLED(i, 0, 0, 255);
// }
// WS2812_Send();
// HAL_Delay(1000);
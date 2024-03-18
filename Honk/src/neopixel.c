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

// Set up SPI:
#define WS2812_SPI SPI2
#define WS2812_SPI_GPIO_PORT GPIOB // GPIOB for SPI2
#define WS2812_SPI_SCK_PIN GPIO_PIN_13  // PB13 for SPI2_SCK
#define WS2812_SPI_MOSI_PIN GPIO_PIN_15 // PB15 for SPI2_MOSI

SPI_HandleTypeDef hspi; // SPI handle

/* WS2812 SPI initialization function */
void WS2812_SPI_Init(void)
{
    /* SPI peripheral clock enable */
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* SPI SCK and MOSI GPIO pin configuration */
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = WS2812_SPI_SCK_PIN | WS2812_SPI_MOSI_PIN; // SCK and MOSI pins
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; // Alternate function push-pull
    GPIO_InitStruct.Pull = GPIO_PULLUP; // Pull-up
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; // High speed
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2; // Alternate function 5 for SPI2
    HAL_GPIO_Init(WS2812_SPI_GPIO_PORT, &GPIO_InitStruct);

    /* SPI initialization */
    hspi.Instance = WS2812_SPI;
    hspi.Init.Mode = SPI_MODE_MASTER;
    hspi.Init.Direction = SPI_DIRECTION_1LINE; // 1-line bidirectional data mode
    hspi.Init.DataSize = SPI_DATASIZE_8BIT; // 8-bit data
    hspi.Init.CLKPolarity = SPI_POLARITY_LOW; // Clock polarity low
    hspi.Init.CLKPhase = SPI_PHASE_1EDGE; // Clock phase 1st edge
    hspi.Init.NSS = SPI_NSS_SOFT; // Software secondary management
    hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // Adjust this as necessary for your LED timing
    hspi.Init.FirstBit = SPI_FIRSTBIT_MSB; // MSB first
    hspi.Init.TIMode = SPI_TIMODE_DISABLE; // Disable TI mode
    hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE; // Disable CRC calculation, CRC is cyclic redundancy check
    hspi.Init.CRCPolynomial = 10039; // previously 10
    HAL_SPI_Init(&hspi);
}

// RGB functionality

// set up LED color storage
#define NUM_LED 22 // number of LEDs
uint8_t LED_Data[NUM_LED][4]; // 4 bytes per LED: G, R, B, 0
uint8_t DEFAULT_RGB[] = {255, 0, 0}; // default color: red

// brightness control on if 1, off if 0
#define USE_BRIGHTNESS 1

// values 0-100 (%), sets brightness of pixels.
//      Affects ws2812_spi()
int brightness = 20;

// set the default RGB values
void setDefaultRGB(int RED, int GREEN, int BLUE)
{
    DEFAULT_RGB[0] = RED <= 255 ? RED : 255;
    DEFAULT_RGB[1] = GREEN <= 255 ? GREEN : 255;
    DEFAULT_RGB[2] = BLUE <= 255 ? BLUE : 255;
}

// get the default RGB values
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
    // if led is out of range or the color values are out of range, return
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
    // WS2812B wants GRB order
    uint32_t color = GREEN << 16 | RED << 8 | BLUE;
    uint8_t sendData[24];
    int indx = 0;

    for (int i = 23; i >= 0; i--)
    // for each bit in the 24-bit color
    {
        if (((color >> i) & 0x01) == 1)
        // if the bit is 1, store 1
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
        // send the RGB data to the LED
    }
    // HAL_Delay(1); ????? Can we safely remove? I think so!
}

// pulser variables
int runningPulse = FALSE; // flag for if a pulse is currently running
int pulse_propagation = 0; // current position of the pulse

uint32_t lastUpdate = 0;
uint32_t tempTime = 0;

// check for RGB LED animation status
void spellPulse(spell used_spell, int start)
{
    // save current time
    tempTime = TIMERS_GetMilliSeconds();

    // if start is true, start a new pulse
    if (start)
        runningPulse = TRUE;

    // if runningPulse is true and the time since the last update is greater than 10ms
    if (runningPulse && tempTime > (lastUpdate + 10))
    {
        // start new pulse
        if (pulse_propagation >= NUM_LED)
        {
            runningPulse = FALSE;
            setLED(pulse_propagation, DEFAULT_RGB[0], DEFAULT_RGB[1], DEFAULT_RGB[2]);
            pulse_propagation = 0;
        }
        else
        {
            // increment pulse_propagation
            pulse_propagation++;
            // switch based on the spell used
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

    int S = 100;
    int L = 50;
    C = ((100.0 - abs(2.0 * L - 100)) * S) * 256 / 100 / 100;
    // C is the chroma value
    H = abs(inAngle) * 120 / 60;
    // H is the hue value
    X = (C * (100 - abs((H % 200) - 100))) * 256 / 100 / 100;
    // X is the secondary color value
    m = (L - C / 2) * 255 / 100;
    // m is the minimum value
    if (H / 120 >= 5) // if hue is greater than 5
    {
        R = C + m;
        G = 0 + m;
        B = X + m;
    }
    else if (H / 120 >= 4) // if hue is greater than 4
    {
        R = X + m;
        G = 0 + m;
        B = C + m;
    }
    else if (H / 120 >= 3) // if hue is greater than 3
    {
        R = 0 + m;
        G = X + m;
        B = C + m;
    }
    else if (H / 120 >= 2) // if hue is greater than 2
    {
        R = 0 + m;
        G = C + m;
        B = X + m;
    }
    else if (H / 120 >= 1) // if hue is greater than 1
    {
        R = X + m;
        G = C + m;
        B = 0 + m;
    }
    else // if hue is less than 1
    {
        R = C + m;
        G = X + m;
        B = 0 + m;
    }
}
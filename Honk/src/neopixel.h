/*
* File: neopixel.h
* Author: Eliot Wachtel, Kevin Jiang
*
* Library for the WS2812B RGB LED
*
* Created on 03/07/2024
*/

#ifndef NEOPIXEL_H
#define NEOPIXEL_H

typedef enum
{
    ROCK,
    PAPER,
    SISSORS,
    UNSPECIFIED
} spell;

/**
 * @Function WS2812_SPI_Init
 * @brief Initializes the SPI peripheral for the WS2812B
 * @return None
*/
void WS2812_SPI_Init(void);

/**
 * @Function WS2812_Send
 * @brief Sends the RGB data to the WS2812B
 * @param data: pointer to the RGB data
 * @param length: length of the data
 * @return None
*/
void spellPulse(spell used_spell, int start);

/**
 * @Function setDefaultRGB
 * @brief Sets the default RGB values for the WS2812B
 * @param RED: red value
 * @param GREEN: green value
 * @param BLUE: blue value
 * @return None
*/
void setDefaultRGB(int RED, int GREEN, int BLUE);

/**
 * @Function getDefaultRGB
 * @brief Gets the default RGB values for the WS2812B
 * @param RED: pointer to red value
 * @param GREEN: pointer to green value
 * @param BLUE: pointer to blue value
 * @return None
*/
void getDefaultRGB(int *RED, int *GREEN, int *BLUE);

#endif /* NEOPIXEL_H */
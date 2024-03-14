#include <stdio.h>
#include <stdint.h>
#include <Board.h>
#include <buttons.h>
#include "stm32f4xx_hal.h"

/**
 * @function BUTTONS_Init(void)
 * @param None
 * @return None
 * @brief Initializes GPIO inputs for the four push buttons on the IO shield
 * @author Adam Korycki, 2023.11.15 */
void BUTTONS_Init(void) {
    BOARD_Init();

    //enable GPIO clocks for ports C and D
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    //init GPIO inputs for buttons
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_12;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/**
 * @function buttons_state(void)
 * @param None
 * @return byte where the lower 4-bits represent the state of the buttons (lsb -> button_0)
 * @brief returns state of push buttons (low bit = button pressed) (eg 0xF == no buttons pressed)
 * @author Adam Korycki, 2023.11.15 */
uint8_t buttons_state(void) {
    uint8_t state = 0;
    state += HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_4);
    state = state << 1;
    state += HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_5);
    state = state << 1;
    state += HAL_GPIO_ReadPin (GPIOC, GPIO_PIN_12);
    state = state << 1;
    state += HAL_GPIO_ReadPin (GPIOD, GPIO_PIN_2);
    return state;
}

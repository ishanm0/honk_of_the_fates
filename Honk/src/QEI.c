#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <pwm.h>
#include <QEI.h>
#include <stdbool.h>
#include <timers.h>

#define MAX_DEGREE 360
#define MAX_RED 255
#define MAX_GREEN 255
#define MAX_BLUE 255

// flags for rotary encoder
volatile int rotary_count = 0; // volatile because it is modified in an ISR
volatile bool rotary_change = false;

void QEI_IRQ();

typedef enum
{
    STATE_00,
    STATE_01,
    STATE_11,
    STATE_10
} QEI_State;

QEI_State state = STATE_11;
uint8_t ENC_state = 0;
int TickCount = 0;
int IncCount = 0;

void QEI_Init(void)
{
    // Configure GPIO pins : PB4 PB5
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // EXTI interrupt init
    HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    // the rest of the function goes here
}

// external interrupt ISR for pin PB5
void EXTI9_5_IRQHandler(void)
{
    // EXTI line interrupt detected
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5); // clear interrupt flag
        QEI_IRQ();
    }
}

// external interrupt ISR for pin PB4
void EXTI4_IRQHandler(void)
{
    // EXTI line interrupt detected
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_4) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4); // clear interrupt flag
        QEI_IRQ();
    }
}

void QEI_IRQ()
{
    // state machine of your design
    static unsigned char rotary_state = 0b11; // initial state is 11 since both pins are high
    unsigned char new_state = 0;              // flip flop state for rotary encoder
    new_state = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) | (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) << 1));
    // mask to 4 bits because 4 bits is max for rotary_state
    // OR the two pins together and shift the second pin to the left by 1 for the new state
    static int buffer = 0; // buffer for rotary encoder for confidence

    // states of state machine: 0, 1, 2, 3
    // if 00 -> 01 -> 11 -> 10 -> 00, then count up
    // if 00 -> 10 -> 11 -> 01 -> 00, then count down
    // if B is low, then counter clockwise (CCW)
    // if A is low, then clockwise (CW)

    switch (rotary_state)
    {
    case 0x00: // 00
        // printf("00\n");
        if (new_state == 0x01)
        { // from 00 to 01, increment counter
            buffer--;
        }
        else if (new_state == 0x02)
        { // from 00 to 10, increment counter
            buffer++;
        }
        break;
    case 0x01: // 01
        // printf("01\n");
        if (new_state == 0x00)
        { // from 01 to 00, increment counter
            buffer++;
        }
        else if (new_state == 0x03)
        { // from 01 to 11, decrement counter
            buffer--;
        }
        break;
    case 0x03: // 11
        // printf("11\n");
        if (new_state == 0x02)
        { // from 11 to 10, decrement counter
            buffer--;
        }
        else if (new_state == 0x01)
        { // from 11 to 01, increment counter
            buffer++;
        }
        break;
    case 0x02: // 10
        // printf("10\n");
        if (new_state == 0x03)
        { // from 10 to 11, increment counter
            buffer++;
        }
        else if (new_state == 0x00)
        { // from 10 to 00, decrement counter
            buffer--;
        }
        break;
    }
    if (buffer == 4)
    {   // if buffer is 4, then decrement rotary count
        // buffer is size of 4 because the rotary encoder has to be changed 4 times to be confident of a new count
        rotary_count--;
        buffer = 0;
    }
    else if (buffer == -4)
    { // if buffer is -4, then increment rotary count
        rotary_count++;
        buffer = 0;
    }

    rotary_state = new_state; // update state

    if (rotary_count == 8)
    { // if rotary count is 24, then reset to 0 since full rotation
        rotary_count = 0;
    }
    else if (rotary_count == -8)
    {
        rotary_count = 0;
    }
}

int QEI_GetPosition(void)
{
    // return current position of encoder in degrees
    // if rotary count is +360, then reset to 0
    // if rotary count is -360, then reset to 0
    return abs(rotary_count * 360 / 8);
}

void QEI_SetPosition(int position)
{
    // reset encoder to 0
    rotary_count = position;
}

void map2color(int *red, int *green, int *blue, int degree)
{
    // map rotary encoder 8 increments to 8 colors
    // 0 -> navy
    // 45 -> rose
    // 90 -> green
    // 135 -> red
    // 180 -> cyan
    // 225 -> yellow
    // 270 -> purple
    // 315 -> white
    if (degree >= 0 && degree < 45)
    {
        *red = 255;
        *green = 10;
        *blue = 255;
    }
    else if (degree >= 45 && degree < 90)
    {
        *red = 0;
        *green = 100;
        *blue = 200;
    }
    else if (degree >= 90 && degree < 135)
    {
        *red = 255;
        *green = 255;
        *blue = 0;
    }
    else if (degree >= 135 && degree < 180)
    {
        *red = 0;
        *green = 255;
        *blue = 255;
    }
    else if (degree >= 180 && degree < 225)
    {
        *red = 255;
        *green = 0;
        *blue = 0;
    }
    else if (degree >= 225 && degree < 270)
    {
        *red = 0;
        *green = 255;
        *blue = 0;
    }
    else if (degree >= 270 && degree < 315)
    {
        *red = 150;
        *green = 0;
        *blue = 255;
    }
    else if (degree >= 315 && degree < 360)
    {
        *red = 0;
        *green = 0;
        *blue = 0;
    }
}
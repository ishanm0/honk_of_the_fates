#include <IR.h>
#include <stdio.h>

unsigned int tmp = 0;
unsigned int time = 0;
int count = 0;
char Bool_flag;

// Initialize IR
void IR_Init(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    // Configure GPIO pin PB5
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_2; //change this in main code
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct); //change this in main code

    // EXTI interrupt init
    HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);

    // the rest of the function goes here
}

// external interrupt ISR for rising edge of pin PD2
void EXTI2_IRQHandler(void)
{
    // printf("gasp\n");
    // EXTI line interrupt detected
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_2) != RESET) //change this too
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2);
        // printf("help\n");
        // anything that needs to happen on falling edge of PB5 (ENC_B)
        // pulse_cnt++;
        tmp = TIMERS_GetMilliSeconds();
        if (tmp >= time + 300)
        {
            count = 1;
            time = tmp;
        }
        else if (tmp >= time + 100)
        {
            count += 1;
            time = tmp;
        }
        Bool_flag = TRUE;
    } else {
        __NOP();
    }
}

/**
 * @function    CAPTOUCH_IsTouched(void)
 * @brief       Returns TRUE if finger is detected. Averaging of previous measurements
 *              may occur within this function, however you are NOT allowed to do any I/O
 *              inside this function.
 * @return      TRUE or FALSE
 */

int IR_Count(void)
{
    return count;
}
int IR_timecheck(void)
{
    return time;
}

char IR_Detect(void)
{
    if (Bool_flag == TRUE)
    {
        Bool_flag = FALSE;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
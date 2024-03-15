#include <IR.h>

#define WINDOW_SIZE 25 // Adjust as needed

int filtered_data[WINDOW_SIZE];

unsigned int tmp = 0;
unsigned int time = 0;
int count = 0;
int flag = 0;
int idx = 0;
int sum = 0;
// int pulse_cnt = 0;
char Bool_flag;

int filter_data(int new_value) // moving average filter, takes a adc value and outputs the moving average value.
{
    filtered_data[idx] = new_value;
    idx++;
    if (idx >= WINDOW_SIZE)
    {
        idx = 0;
    }
    sum = 0;
    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        sum += filtered_data[i];
    }
    return sum / WINDOW_SIZE;
}

void IR_Init(void)
{
    // Configure GPIO pin PB5
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // EXTI interrupt init
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    // the rest of the function goes here
}

// external interrupt ISR for rising edge of pin PB5
void EXTI9_5_IRQHandler(void)
{
    // EXTI line interrupt detected
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5); // clear interrupt flag

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
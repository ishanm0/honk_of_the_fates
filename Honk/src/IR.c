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

// pulser
int runningIRpulses = FALSE; // is a pulse animation active
int signal_propegation = 0;
int ON_TIME = 50; // microseconds to have pwm signal on
int OFF_TIME = 100; // microseconds to have pwm signal off between sucessive on pules
int MIN_WAIT_BETWEEN_PULSES = 500; // minimum microseconds between unique complete signals

uint32_t lastUpdate = 0;
uint32_t tempTime = 0;
uint32_t currDelay = 0;
// uint32_t TIMERS_GetMilliSeconds(void);

// PWM_Start(PWM_4);
// HAL_Delay(50);
// PWM_Stop(PWM_4);
// HAL_Delay(100);
// when called will either start a new IR signal or service the active one (depending on if there is an active one).
int sendIRsignal(int playerID){
    if(playerID == 0) return 0; // if input is 0, do not do anything
    
    // check current time
    tempTime = TIMERS_GetMilliSeconds();
    // ...
    
    if (runningIRpulses && tempTime >= (lastUpdate + currDelay))
    {
        // printf("Yes1: %d : %d\n", tempTime, lastUpdate);
        if (signal_propegation >= (playerID * 3))
        {
            runningIRpulses = FALSE;
            PWM_Stop(PWM_5); // double check that it is stopped
            signal_propegation = 0;
            currDelay = MIN_WAIT_BETWEEN_PULSES;
            return TRUE;
        }
        
        if(currDelay == ON_TIME){
            // turns off the pwm to give a pause time
            PWM_Stop(PWM_5);
            currDelay = OFF_TIME;
            signal_propegation ++; // increment to track the newly completed pulse.
        } else if(currDelay == OFF_TIME){
            // Turns the pwm on to generate a pulse.
            PWM_Start(PWM_5);
            currDelay = ON_TIME;
        }
        // update last update time store
        lastUpdate = TIMERS_GetMilliSeconds();
    } else if (!runningIRpulses && tempTime > (lastUpdate + MIN_WAIT_BETWEEN_PULSES)){
        // start a new pulse
        runningIRpulses = TRUE;
        PWM_Start(PWM_5);
        currDelay = ON_TIME;
        signal_propegation = 0;
        lastUpdate = TIMERS_GetMilliSeconds();
    }
    return FALSE;
}

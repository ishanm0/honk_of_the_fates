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
    GPIO_InitStruct.Pin = GPIO_PIN_2; 
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct); 

    // EXTI interrupt init
    HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);
}

// external interrupt ISR for rising edge of pin PD2
void EXTI2_IRQHandler(void)
{
    // EXTI line interrupt detected
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_2) != RESET) //change this too
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2); // clear interrupt flag
        tmp = TIMERS_GetMilliSeconds(); // get the current time
        if (tmp >= time + 300) // if the time difference is greater than 300ms
        {
            count = 1; // reset count
            time = tmp;
        }
        else if (tmp >= time + 100) // if the time difference is greater than 100ms
        {
            count += 1; // increment count
            time = tmp;
        }
        Bool_flag = TRUE;
    } else {
        __NOP(); // do nothing
    }
}

// returns the count of the number of times the IR sensor has been triggered
int IR_Count(void)
{
    return count;
}

// returns the time of the last IR sensor trigger
int IR_timecheck(void)
{
    return time;
}

// returns TRUE if the IR sensor has been triggered
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

uint32_t lastIRUpdate = 0;
uint32_t tempIRTime = 0;
uint32_t currDelay = 0;

// when called will either start a new IR signal or service the active one (depending on if there is an active one).
int sendIRsignal(int playerID){
    if(playerID == 0) return 0; // if input is 0, do not do anything
    
    // check current time
    tempIRTime = TIMERS_GetMilliSeconds();
    // ...
    
    if (runningIRpulses && tempIRTime >= (lastIRUpdate + currDelay))
    {
        // printf("Yes1: %d : %d\n", tempIRTime, lastIRUpdate);
        if (signal_propegation >= (playerID * 3))
        // if the signal has been propagated enough times
        {
            runningIRpulses = FALSE;
            PWM_Stop(PWM_5); // double check that it is stopped
            signal_propegation = 0;
            currDelay = MIN_WAIT_BETWEEN_PULSES; // reset the delay
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
        lastIRUpdate = TIMERS_GetMilliSeconds();
    } else if (!runningIRpulses && tempIRTime > (lastIRUpdate + MIN_WAIT_BETWEEN_PULSES)){
        // start a new pulse
        runningIRpulses = TRUE;
        PWM_Start(PWM_5);
        currDelay = ON_TIME;
        signal_propegation = 0;
        lastIRUpdate = TIMERS_GetMilliSeconds();
    }
    return FALSE;
}

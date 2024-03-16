#include <stdio.h>
#include <stdlib.h>
#include <Board.h>

#include <ADC.h>
#include <PWM.h>
#include <buttons.h>

#include <math.h>

#define TONE_196 196
#define TONE_293 293
#define TONE_440 440
#define TONE_659 659

// set up buttons
#define NO_BUTTON 0xF// 0b1111
#define BUTTON_A 0xE // 0b1110
#define BUTTON_B 0xD // 0b1101
#define BUTTON_C 0xB // 0b1011
#define BUTTON_D 0x7 // 0b0111

int ADCMin = 0;
// int ADCMax = 0xfff;
int ADCMax = 200;
int PWMMin = 0;
int PWMMax = 100;
int PWMFreqMin = 100;
int PWMFreqMax = 3000;

typedef struct {
    int QueueSize;      // how much of the queue to use
    int movingAverage;     // Newly calculated Average
    int queue[300];     // the queue storing the last QueueSize worth of values (max 100)
    int queuePosition;  // current position last updated in the queue
} circularBuffer;

// adds new value to the circular buffer
int moveAverage(circularBuffer* currBuffer, int newValue){
    currBuffer->queue[currBuffer->queuePosition] = newValue; // add new value to the queue
    currBuffer->queuePosition ++;
    if (currBuffer->queuePosition == currBuffer->QueueSize){
        currBuffer->queuePosition = 0;
    }
    return 0;
}

// gets the average value of the circular buffer
int getAverage(circularBuffer* currBuffer){
    int summing = 0;
    for(int i = 0; i < currBuffer->QueueSize;i++){
        summing += currBuffer->queue[i];
    }
    currBuffer->movingAverage = summing/currBuffer->QueueSize;
    return(currBuffer->movingAverage);
}

// checks if two items are at least buff different from each other
int bufferedDiffereceCheck(int itemA, int itemB, int buff){
    if(itemA > itemB && itemA - itemB > buff){
        return 1;
    } else if(itemA < itemB && itemB - itemA > buff){
        return 1;
    } else {
        return 0;
    }
}

// maps from ADC range to PWM range
int ADCtoPWMrange(int OldValue){
    return(((OldValue - ADCMin) * (PWMMax - PWMMin)) / (ADCMax - ADCMin)) + PWMMin;
}

// maps from ADC range to PWM range
int ADCtoPWMfreqRange(int OldValue){
    return(((OldValue - ADCMin) * (PWMFreqMax - PWMFreqMin)) / (ADCMax - ADCMin)) + PWMFreqMin;
}

// maps ADC to approximate linearity - didn't work because actually didn't need
float ADCtoLinearRange(int ADC_value){
    // use sqrtf(); instead of sqrt https://community.st.com/t5/stm32-mcus-products/stm32f4-fpu-square-root-in-c/td-p/526737
    // Model: $ADCreading = 0.11x^{2} - 11*sensorAngle + 575$ for 575 >= angle >= 300 
    // Model thus is: $sensorAngle = 0.91(55-3.3\sqrt{y-300})$
    if(ADC_value > 516){
        // above model boundaries = 0 angle
        return( 0 );
    } else if(ADC_value < 156){
        // below model boundaries = >50 degree angle
        // I may want to use a linear model for this one 
        //    ^ if I find myself noticibly bottoming out
        return( 200 );
    }
    // return( 0.91*(55 - 3.3*sqrtf(ADC_value-300)) );
    return( 2.5*(85 - 2.236*sqrtf(4*ADC_value-619)) );
}


// a function, inspired by Ishan Madan, 
//  to turn the approximately linear potentiometer signal
//  into a logarithmic signal.
int loggarithmatizeADC(int ADCin){
    // calcuate range I am translating to
    int audioScalar = PWMFreqMax - PWMFreqMin;
    
    // safety precaution since values are no longer hardware limited
    if(ADCin > 200){
        printf("ADC exceeds expectation");
        return(PWMFreqMin);
    }

    // (audioScalar^{1/ADC_value_range})^ADCin + PWMFreqMin
    // translated the ADC derived input into an audio frequency on a log scale
    return((int)pow(pow(audioScalar, 1.0 / 200), ADCin) + PWMFreqMin);
    // return(42*(int)pow(pow(5000, 1.0 / 400), ADCin) + 70);
}

// pulser
int runningIRpulses = FALSE; // is s pulse animation active
int signal_propegation = 0;
int ON_TIME = 50; // microseconds to have pwm signal on
int OFF_TIME = 100; // microseconds to have pwm signal off between sucessive on pules
int MIN_WAIT_BETWEEN_PULSES = 500 // minimum microseconds between unique complete signals

uint32_t lastUpdate = 0;
uint32_t tempTime = 0;
uint32_t currDelay = 0;
// uint32_t TIMERS_GetMilliSeconds(void);

// PWM_Start(PWM_4);
// HAL_Delay(50);
// PWM_Stop(PWM_4);
// HAL_Delay(100);
// when called will either start a new IR signal or service the active one (depending on if there is an active one).
void sendIRsignal(int payerID){
    // check current time
    tempTime = TIMERS_GetMilliSeconds();
    // ...
    if (runningIRpulses && tempTime > (lastUpdate + currDelay))
    {
        // printf("Yes1: %d : %d\n", tempTime, lastUpdate);
        if (signal_propegation >= (playerID * 3))
        {
            runningIRpulses = FALSE;
            PWM_Stop(PWM_4); // double check that it is stopped
            signal_propegation = 0;
            currDelay = MIN_WAIT_BETWEEN_PULSES;
        }
        
        switch(currDelay){
            case ON_TIME:
                // Turns the pwm on to generate a pulse.
                PWM_Start(PWM_4);
                
                break;
            case OFF_TIME:
                // st;
                break;
        }
        // update last update time store
        lastUpdate = TIMERS_GetMilliSeconds();
    } else if (tempTime > (lastUpdate + 500)){
        // start a new pulse
        runningIRpulses = TRUE;
        PWM_Start(PWM_4);
        currDelay = ONTIME;
        signal_propegation = 0;
    }
}

// so you can do it without a set spell
// void spellPulse(void){
//     spellPulse(UNSPECIFIED);
// }

// MAIN:

// int R, G, B, C, X, H_prime;
char flag;
int initFunc(void) // initializes everything we need
{
    BOARD_Init();
    // PWM_Init();
    TIMER_Init();

    // set up PWM
    PWM_Init();
    PWM_SetDutyCycle(PWM_4, 50); // seems like a reasonable place to put it
    PWM_SetFrequency(30000);
    // PWM_Stop(PWM_4); // stop the sound until it is activated in loop

    return SUCCESS;
}


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

int main(void) {
    initFunc();
    
    while (TRUE) {
        // printf("Hello World\r\n");
        HAL_Delay(500);
        
        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);

        HAL_Delay(500);
        
        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);

        HAL_Delay(500);
        
        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);

        HAL_Delay(500);
        
        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);
        HAL_Delay(100);

        PWM_Start(PWM_4);
        HAL_Delay(50);
        PWM_Stop(PWM_4);

        // printf(">angle:%ld:%d\n", timeStamp, angle);
        // printf(">ADC:%d\n", readADC);
    }

}

// int main(void) {
//     BOARD_Init();
    
//     // set up ADC
//     ADC_Init();
//     circularBuffer ADCqueue;
//     ADCqueue.movingAverage = 0;
//     ADCqueue.QueueSize = 200;
//     ADCqueue.queuePosition = 0;
//     // set up PWM
//     PWM_Init();
//     PWM_SetDutyCycle(PWM_0, 50); // seems like a reasonable place to put it
//     PWM_Stop(PWM_0); // stop the sound until it is activated in loop
//     // set up buttons:
//     BUTTONS_Init();
//     // uint8_t recordedButtonState = NO_BUTTON;

//     // ADC value tracking variables
//     int readADC = 0;
//     int currPWMfreq = 100;
//     // for storing the status of audio playback session
//     int playbackMode = 0;
//     // number of playback cycles waited till playback turn off
//     int PLAYBACK_CYCLES = 1000; 

//     // frequency related variables:
//     circularBuffer FREQqueue;
//     FREQqueue.movingAverage = 0;
//     FREQqueue.QueueSize = 50;
//     FREQqueue.queuePosition = 0;
//     // printf("Hello World\r\n");
    
//     while (TRUE) {
//         // printf("Hello World\r\n");

//         // read new ADC value
//         readADC = ADC_Read(ADC_0);
//         moveAverage(&ADCqueue, readADC);
//         // Three steps:
//         // 1) get the new average ADC reading
//         // 2) translate it to linear range
//         // 3) translate that to a nicer sounding log range
//         currPWMfreq = loggarithmatizeADC(ADCtoLinearRange(getAverage(&ADCqueue)));
//         // currPWMfreq = ADCtoPWMfreqRange(ADCtoLinearRange(getAverage(&ADCqueue)));
        
//         moveAverage(&FREQqueue, currPWMfreq);
//         currPWMfreq = getAverage(&FREQqueue);

//         // readADC = ADC_Read(ADC_0);
//         // // Three steps:
//         // // 1) get the new average ADC reading
//         // // 2) translate it to linear range
//         // // 3) translate that to a nicer sounding log range
//         // moveAverage(&ADCqueue, loggarithmatizeADC(ADCtoLinearRange(readADC)));
//         // currPWMfreq = getAverage(&ADCqueue);

//         readADC = ADC_Read(ADC_1);
        
//         if(playbackMode == 0){
//             if(readADC > 30){
//                 PWM_Start(PWM_0);
//                 PWM_SetFrequency(currPWMfreq);
//                 playbackMode = 1;
//             }
//         } else if(playbackMode < PLAYBACK_CYCLES){
//             PWM_SetFrequency(currPWMfreq);
//             if(readADC > 30){
//                 playbackMode = 1;
//             } else {
//                 playbackMode ++;
//             }
//         } else if(playbackMode >= PLAYBACK_CYCLES){
//             PWM_Stop(PWM_0);
//             playbackMode = 0;
//         }

//         // printf("POT   = %d\r\n", readADC);
//         // printf(">angle:%ld:%d\n", timeStamp, angle);
//         printf(">ADC:%d\n", readADC);
//         // printf(">ADC:%d\n", ADC_Read(ADC_1));
//     }

//     // ADC_End();
// }
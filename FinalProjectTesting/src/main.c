#include <IR.h>
#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <ADC.h>
#include <pwm.h>
#include <math.h>

int R, G, B, C, X, H_prime;
char temp_check;
int initFunc(void) // initializes everything we need
{
    BOARD_Init();
    ADC_Init();
    PWM_Init();
    IR_Init();
    TIMER_Init();
    return SUCCESS;
}

// takes a value x that ranges from in_min to in_max
// outputs a the value mapped from a range of out_min to out_max
int map(int x, int in_min, int in_max, int out_min, int out_max)
{
    int scaled_value = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

    // Ensure the result does not exceed the specified range
    if (scaled_value > out_max)
    {
        scaled_value = out_max;
    }
    // Ensure the result does not go below the specified range
    else if (scaled_value < out_min)
    {
        scaled_value = out_min;
    }

    return scaled_value;
}

int main(void)
{
    initFunc();
    temp_check = FALSE;
    while (TRUE)
    {


        if (temp_check == FALSE && IR_Detect() == TRUE){
            printf("IsTouched: TRUE %d \n", IR_Count());
            temp_check = TRUE;
        } else if (temp_check == TRUE && IR_Detect() == FALSE){
            printf("IsTouched: FALSE \n");
            temp_check = FALSE;
        }

        
    }
}
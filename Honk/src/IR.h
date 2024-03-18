/**
 * @file    IR.h
 * @brief   IR sensing module
 * @author  Kevin Jiang
 * @date    March 11, 2023
 * @detail  This module uses the external interrupt peripheral to detect IR signal inputs.
 *          The peripheral is configured to generate an interrupt every rising edge 
 *          of pin PB5 (ENC_B) which means that the difference of two interrupts gives 
 *          you the period of the signal. Use the timers.h library for timing operations.
 *          The IR sensor is connected to pin PB5.
 */

#ifndef IR_H
#define IR_H

/*******************************************************************************
 * INCLUDES                                                                    *
 ******************************************************************************/
#include <Board.h>
#include <timers.h>
#include <pwm.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include <stm32f4xx_hal_tim.h>

/**
 * @function    IR_Init(void)
 * @brief       This function initializes the module for use. Initialization is 
 *              done by opening and configuring timer 2, opening and configuring the GPIO pin
 *              and setting up the interrupt.
 * @return      None
 */
void IR_Init(void);

/**
 * @function    IR_Detect(void)
 * @brief       Returns TRUE if finger is detected. Averaging of previous measurements 
 *              may occur within this function, however you are NOT allowed to do any I/O
 *              inside this function.
 * @return      TRUE or FALSE
 */
char IR_Detect(void);

/**
 * @function    IR_Detect(void)
 * @brief       Returns count. 
 */
int IR_Count(void);
int IR_timecheck(void);

// serivceing function for sending IR signal
int sendIRsignal(int);

#endif  /* IR_H */
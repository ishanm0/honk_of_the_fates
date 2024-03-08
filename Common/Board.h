/*
 * File:   Board.h
 * Author: Adam Korycki
 *
 * Created on September 3, 2023
 */

#ifndef BOARD_H
#define	BOARD_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

// Boolean defines for TRUE, FALSE, SUCCESS and ERROR
#ifndef FALSE
#define FALSE ((int8_t) 0)
#define TRUE ((int8_t) 1)
#endif
#ifndef ERROR
#define ERROR ((int8_t) -1)
#define SUCCESS ((int8_t) 1)
#endif

/* minimum GPIO pin assocations for NUCLEO64*/
#define B1_Pin GPIO_PIN_13 // onboard blue push button
#define B1_GPIO_Port GPIOC
#define USART_TX_Pin GPIO_PIN_2 // USB-UART (USART2)
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA

/**
 * @function BOARD_Init(void)
 * @param None
 * @return None
 * @brief Set the clocks up for the board, initializes the serial port, and turns on the A/D
 *        subsystem for battery monitoring
 * @author Adam Korycki, 2023.09.03  */
void BOARD_Init();


/**
 * @function BOARD_End(void)
 * @param None
 * @return None
 * @brief Shuts down all peripherals except for serial and A/D. Turns all pins into input
 * @author Adam Korycki, 2023.09.03  */
void BOARD_End();

#endif	/* BOARD_H */


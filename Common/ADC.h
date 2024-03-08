/*
 * File:   ADC.h
 * Author: Adam Korycki
 *
 * Created on September 3, 2023
 */

#ifndef ADC_H
#define	ADC_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc.h"

ADC_HandleTypeDef hadc1;

#define ADC_0 ADC_CHANNEL_0
#define ADC_1 ADC_CHANNEL_1
#define POT   ADC_CHANNEL_4   // onboard potentiometer
#define ADC_2 ADC_CHANNEL_10
#define ADC_3 ADC_CHANNEL_11
#define ADC_4 ADC_CHANNEL_12
#define ADC_5 ADC_CHANNEL_13

/**
 * @function ADC_Init(void)
 * @param None
 * @return SUCCESS or ERROR
 * @brief Initializes the ADC subsystem
 * @author Adam Korycki, 2023.09.16 */
char ADC_Init(void);

/**
 * @function ADC_End(void)
 * @param None
 * @return None
 * @brief Disables the A/D subsystem and release the pins used
 * @author Adam Korycki, 2023.09.16 */
void ADC_End(void);

/**
 * @function ADC_Read(uint32_t channel)
 * @param channel - select ADC channel (ADC_0, ADC_1, ..., ADC_5, POT)
 * @return 12-bit ADC reading
 * @brief selects ADC channel and returns 12-bit reading
 * @author Adam Korycki, 2023.09.16 */
uint32_t ADC_Read(uint32_t channel);


#endif

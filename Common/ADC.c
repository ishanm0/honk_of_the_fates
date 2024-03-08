/*
 * File:   ADC.c
 * Author: Adam Korycki
 *
 * Created on September 16, 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "ADC.h"

// Boolean defines for TRUE, FALSE, SUCCESS and ERROR
#ifndef FALSE
#define FALSE ((int8_t) 0)
#define TRUE ((int8_t) 1)
#endif
#ifndef ERROR
#define ERROR ((int8_t) -1)
#define SUCCESS ((int8_t) 1)
#endif

static uint8_t init_status = FALSE;

/**
 * @function ADC_Init(void)
 * @param None
 * @return SUCCESS or ERROR
 * @brief Initializes the ADC subsystem with Interrupt, selects ADC pin 0 by default
 * @author Adam Korycki, 2023.09.16 */
char ADC_Init(void) {
	if (init_status == FALSE) { // if ADC has not been initialized
		ADC_ChannelConfTypeDef sConfig = {0};
		/** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) */
		hadc1.Instance = ADC1;
		hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
		hadc1.Init.Resolution = ADC_RESOLUTION_12B;
		hadc1.Init.ScanConvMode = DISABLE;
		hadc1.Init.ContinuousConvMode = ENABLE;
		hadc1.Init.DiscontinuousConvMode = DISABLE;
		hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
		hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
		hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
		hadc1.Init.NbrOfConversion = 1;
		hadc1.Init.DMAContinuousRequests = DISABLE;
		hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

		if (HAL_ADC_Init(&hadc1) != HAL_OK) {
			return ERROR;
		}
		
		// by default, select onboard potentiometer
		sConfig.Channel = POT;
		sConfig.Rank = 1;
		sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
		if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
			return ERROR;
		}
		init_status = TRUE;
	}
	return SUCCESS;
}

/**
 * @function AD_End(void)
 * @param None
 * @return None
 * @brief Disables the A/D subsystem and release the pins used
 * @author Adam Korycki, 2023.09.16 */
void ADC_End(void){
	HAL_ADC_Stop(&hadc1);
	HAL_ADC_DeInit(&hadc1);
}

/**
 * @function ADC_Read(uint32_t channel)
 * @param channel - select ADC channel (ADC_0, ADC_1, ..., ADC_5, POT)
 * @return 12-bit ADC reading
 * @brief selects ADC channel and returns 12-bit reading
 * @author Adam Korycki, 2023.09.16 */
uint32_t ADC_Read(uint32_t channel){
	// select channel and sampling time
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = channel;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);
	
	// Start ADC Conversion, wait for conversion and return value
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 1);
	return HAL_ADC_GetValue(&hadc1);
}

//#define ADC_TEST
#ifdef ADC_TEST //ADC TEST HARNESS
// SUCCESS - printed analog values for each channel are correct

#include <stdio.h>
#include <Board.h>
#include <ADC.h>

int main(void) {
	BOARD_Init();
	char ret = ADC_Init();

	if (ret != ERROR) {
		while (TRUE) {
			printf("POT   = %d\r\n", ADC_Read(POT));
			printf("ADC_0 = %d\r\n", ADC_Read(ADC_0));
			printf("ADC_1 = %d\r\n", ADC_Read(ADC_1));
			printf("ADC_2 = %d\r\n", ADC_Read(ADC_2));
			printf("ADC_3 = %d\r\n", ADC_Read(ADC_3));
			printf("ADC_4 = %d\r\n", ADC_Read(ADC_4));
			printf("ADC_5 = %d\r\n\r\n", ADC_Read(ADC_5));
			HAL_Delay(500);
		}
	}
	else {
		printf("ADC init error");
	}
}

#endif

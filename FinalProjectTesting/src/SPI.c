// /*
//  * File:   SPI.c
//  * Author: Eliot Wachtel & Kevin Jiang
//  *
//  * Created on March 14, 2024
//  */

// #include <stdio.h>
// #include <stdlib.h>
// #include <stdint.h>
// #include "stm32f4xx_hal.h"
// #include "stm32f4xx_hal_spi.h"
// #include "SPI.h"

// // Boolean defines for TRUE, FALSE, SUCCESS and ERROR
// #ifndef FALSE
// #define FALSE ((int8_t) 0)
// #define TRUE ((int8_t) 1)
// #endif
// #ifndef ERROR
// #define ERROR ((int8_t) -1)
// #define SUCCESS ((int8_t) 1)
// #endif

// SPI_HandleTypeDef hspi2;

// static uint8_t init_status = FALSE;

// /**
//  * @Function I2C_Init(Rate)
//  * @param None
//  * @return SUCCESS or ERROR
//  * @brief  Initializes the SPI System at standard speed (100Kbps)
//  * @author Eliot Wachtel & Kevin Jiang, 2024.03.14 */
// int8_t SPI_Init(void) {
//     if (init_status == FALSE) { // if SPI interface has not been initialized
//         // hspi2.Instance = I2C2;
//         // hspi2.Init.ClockSpeed = 100000;
//         // hspi2.Init.DutyCycle = I2C_DUTYCYCLE_2;
//         // hspi2.Init.OwnAddress1 = 0;
//         // hspi2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
//         // hspi2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
//         // hspi2.Init.OwnAddress2 = 0;
//         // hspi2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
//         // hspi2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
        
//         if (HAL_SPI_Init(&hspi2) != HAL_OK)
//         {
//             return ERROR;
//         }
//         init_status = TRUE;
//     }
//     return SUCCESS;
// }

// /**
//  * @Function I2C_ReadRegister(unsigned char I2CAddress,unsigned char deviceRegisterAddress)
//  * @param I2CAddresss - 7-bit address of I2C device wished to interact with
//  * @param deviceRegisterAddress - 8-bit address of register on device
//  * @return Value at Register or 0
//  * @brief  Reads one device register on chosen I2C device
//  * @author Adam Korycki, 2023.10.03 */
// unsigned char I2C_ReadRegister(unsigned char I2CAddress,unsigned char deviceRegisterAddress) {
//     HAL_StatusTypeDef ret;
//     I2CAddress = I2CAddress << 1; // use 8-bit address
//     uint8_t* data = &deviceRegisterAddress;

//     // start condition
//     ret = HAL_I2C_Master_Transmit(&hi2c2, I2CAddress, data, 1, HAL_MAX_DELAY); // wait for it to end, this is internal and can not stall 
//     if (ret != HAL_OK) {
//         printf("I2C Tx Error on read start condition\r\n");
//         return 0;
//     }

//     // get byte
//     ret = HAL_I2C_Master_Receive(&hi2c2, I2CAddress, data, 1, HAL_MAX_DELAY); // wait for it to end, this is internal and can not stall
//     if (ret != HAL_OK) {
//         printf("I2C Rx Error on read byte\r\n");
//         return 0;
//     }

//     return *data;
// }

// /**
//  * @Function I2C_WriteReg(unsigned char I2CAddress, unsigned char deviceRegisterAddress, char data)
//  * @param I2CAddresss - 7-bit address of I2C device wished to interact with
//  * @param deviceRegisterAddress - 8-bit address of register on device
//  * @param data - data wished to be written to register
//  * @return 0 if error and 1 if success
//  * @brief  Writes one device register on chosen I2C device
//  * @author Adam Korycki, 2023.10.03 */
// unsigned char I2C_WriteReg(unsigned char I2CAddress, unsigned char deviceRegisterAddress, uint8_t data) {
//     HAL_StatusTypeDef ret;
//     I2CAddress = I2CAddress << 1; // use 8-bit address

//     ret = HAL_I2C_Mem_Write(&hi2c2, I2CAddress, deviceRegisterAddress, I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY);
//     if (ret != HAL_OK) {
//         printf("I2C Tx Error on write data\r\n");
//         return ERROR;
//     }

//     return SUCCESS;   
// }

// /**
//  * @Function I2C_ReadInt(char I2CAddress, char deviceRegisterAddress, char isBigEndian)
//  * @param I2CAddresss - 7-bit address of I2C device wished to interact with
//  * @param deviceRegisterAddress - 8-bit lower address of register on device
//  * @param isBigEndian - Boolean determining if device is big or little endian
//  * @return ERROR or SUCCESS
//  * @brief  Reads two sequential registers to build a 16-bit value. isBigEndian
//  * whether the first bits are either the high or low bits of the value
//  * @author Adam Korycki, 2023.10.04*/
// int I2C_ReadInt(char I2CAddress, char deviceRegisterAddress, char isBigEndian) {
//     short data = 0; // 16-bit return value

//     unsigned char byte = I2C_ReadRegister(I2CAddress, deviceRegisterAddress); // get first byte
//     if (isBigEndian) {
//         data = byte << 8;
//     } else {
//         data = byte;
//     }

//     byte = I2C_ReadRegister(I2CAddress, deviceRegisterAddress + 1); // get second byte
//     if (isBigEndian) {
//         data |= byte;
//     } else {
//         data |= byte << 8;
//     }
//     return data;
// }
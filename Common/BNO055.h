/* 
 * File:   BNO055.h
 * Author: Aaron Hunter, Adam Korycki
 * 
 * Software module to communicate with the IMU over I2C.
 * Provides access to each raw sensor axis along with raw temperature
 *
 * Created on April 11, 2022
 * Updated on October 4, 2023
 */

#ifndef BNO055_H
#define	BNO055_H

#include <stdint.h>

/**
 * @Function BNO055_Init(void)

 * @return ERROR or SUCCESS
 * @brief  Initializes the BNO055 for usage.
 * @author Aaron Hunter */
int8_t BNO055_Init(void);


/**
 * @Function BNO055_Read*()
 * @param None
 * @return Returns raw sensor reading
 * @brief reads sensor axis as given by name
 * @author Aaron Hunter*/
int BNO055_ReadAccelX(void);

/**
 * @Function BNO055_Read*()
 * @param None
 * @return Returns raw sensor reading
 * @brief reads sensor axis as given by name
 * @author Aaron Hunter*/
int BNO055_ReadAccelY(void);

/**
 * @Function BNO055_Read*()
 * @param None
 * @return Returns raw sensor reading
 * @brief reads sensor axis as given by name
 * @author Aaron Hunter*/
int BNO055_ReadAccelZ(void);

/**
 * @Function BNO055_Read*()
 * @param None
 * @return Returns raw sensor reading
 * @brief reads sensor axis as given by name
 * @author Aaron Hunter*/
int BNO055_ReadGyroX(void);

/**
 * @Function BNO055_Read*()
 * @param None
 * @return Returns raw sensor reading
 * @brief reads sensor axis as given by name
 * @author Aaron Hunter*/
int BNO055_ReadGyroY(void);


/**
 * @Function BNO055_Read*()
 * @param None
 * @return Returns raw sensor reading
 * @brief reads sensor axis as given by name
 * @author Aaron Hunter*/
int BNO055_ReadGyroZ(void);



/**
 * @Function BNO055_Read*()
 * @param None
 * @return Returns raw sensor reading
 * @brief reads sensor axis as given by name
 * @author Aaron Hunter*/
int BNO055_ReadMagX(void);

/**
 * @Function BNO055_Read*()
 * @param None
 * @return Returns raw sensor reading
 * @brief reads sensor axis as given by name
 * @author Aaron Hunter*/
int BNO055_ReadMagY(void);

/**
 * @Function BNO055_Read*()
 * @param None
 * @return Returns raw sensor reading
 * @brief reads sensor axis as given by name
 * @author Aaron Hunter*/
int BNO055_ReadMagZ(void);


/**
 * @Function BNO055_Read*()
 * @param None
 * @return Returns raw sensor reading
 * @brief reads sensor axis as given by name
 * @author Aaron Hunter*/
int BNO055_ReadTemp(void);

#endif

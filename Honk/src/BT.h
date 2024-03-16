/*
 * File:  BT.h
 * Author: Indy Spott, Ishan Madan
 *
 * Library functions for the BT module, bluefruit LE module
 *
 * Created on 03/07/2024
 */

#ifndef BT_H
#define BT_H

#include <stdint.h>
#include <uart.h> // uart library from adam

// BT_HandleTypeDef hbt; // BT handle

/**
 * @Function BT_Init
 * @return SUCCESS or ERROR
 */
int BT_Init();

/**
 * @Function BT_Recv
 * @param data: pointer to data buffer
 * @return length of data received or ERROR if no data available
 */
int BT_Recv(uint8_t *data);

/**
 * @Function BT_Send
 * @param data: pointer to data buffer
 * @param length: length of data to send
 * @return SUCCESS or ERROR
 */
int BT_Send(uint8_t *data, int length);

#endif
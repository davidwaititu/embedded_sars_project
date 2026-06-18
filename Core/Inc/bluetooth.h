#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include "stm32l4xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

#define BT_TIMEOUT 100

void BT_SendString(const char *str);
void BT_SendBytes(const uint8_t *pData, uint16_t size);
void BT_Printf(const char *format, ...);

#endif /* __BLUETOOTH_H */
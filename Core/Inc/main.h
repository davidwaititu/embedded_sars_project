/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define UP_BTN_Pin GPIO_PIN_13
#define UP_BTN_GPIO_Port GPIOC
#define MIC_DAT_Pin GPIO_PIN_0
#define MIC_DAT_GPIO_Port GPIOC
#define RIGHT_BTN_Pin GPIO_PIN_1
#define RIGHT_BTN_GPIO_Port GPIOC
#define MIC_CLK_Pin GPIO_PIN_2
#define MIC_CLK_GPIO_Port GPIOC
#define OBI_SPI2_MOSI_Pin GPIO_PIN_3
#define OBI_SPI2_MOSI_GPIO_Port GPIOC
#define USART2_TX_Pin GPIO_PIN_2
#define USART2_TX_GPIO_Port GPIOA
#define USART2_RX_Pin GPIO_PIN_3
#define USART2_RX_GPIO_Port GPIOA
#define LEFT_BTN_Pin GPIO_PIN_0
#define LEFT_BTN_GPIO_Port GPIOB
#define MATRIX_RCK_Pin GPIO_PIN_11
#define MATRIX_RCK_GPIO_Port GPIOB
#define OB_SPI2_SCK_Pin GPIO_PIN_13
#define OB_SPI2_SCK_GPIO_Port GPIOB
#define OB_SPI2_MISO_Pin GPIO_PIN_14
#define OB_SPI2_MISO_GPIO_Port GPIOB
#define SERVO_MOTOR_Pin GPIO_PIN_8
#define SERVO_MOTOR_GPIO_Port GPIOC
#define DOWN_BTN_Pin GPIO_PIN_9
#define DOWN_BTN_GPIO_Port GPIOC
#define USART3_TX_Pin GPIO_PIN_10
#define USART3_TX_GPIO_Port GPIOC
#define USART3_RX_Pin GPIO_PIN_11
#define USART3_RX_GPIO_Port GPIOC
#define LED_1_Pin GPIO_PIN_6
#define LED_1_GPIO_Port GPIOB
#define I2C1_SCL_Pin GPIO_PIN_8
#define I2C1_SCL_GPIO_Port GPIOB
#define I2C1_SDA_Pin GPIO_PIN_9
#define I2C1_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

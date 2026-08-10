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
#include "stm32f4xx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define M_RL13_Pin GPIO_PIN_2
#define M_RL13_GPIO_Port GPIOE
#define M_RL14_Pin GPIO_PIN_3
#define M_RL14_GPIO_Port GPIOE
#define M_RL15_Pin GPIO_PIN_4
#define M_RL15_GPIO_Port GPIOE
#define M_RL16_Pin GPIO_PIN_5
#define M_RL16_GPIO_Port GPIOE
#define M_RL1_Pin GPIO_PIN_6
#define M_RL1_GPIO_Port GPIOE
#define CPU_STA_Pin GPIO_PIN_13
#define CPU_STA_GPIO_Port GPIOC
#define M_RL2_Pin GPIO_PIN_0
#define M_RL2_GPIO_Port GPIOF
#define M_RL3_Pin GPIO_PIN_1
#define M_RL3_GPIO_Port GPIOF
#define M_RL4_Pin GPIO_PIN_2
#define M_RL4_GPIO_Port GPIOF
#define M_RL5_Pin GPIO_PIN_3
#define M_RL5_GPIO_Port GPIOF
#define M_RL6_Pin GPIO_PIN_4
#define M_RL6_GPIO_Port GPIOF
#define M_RL7_Pin GPIO_PIN_5
#define M_RL7_GPIO_Port GPIOF
#define M_RL8_Pin GPIO_PIN_6
#define M_RL8_GPIO_Port GPIOF
#define M_IN1_Pin GPIO_PIN_11
#define M_IN1_GPIO_Port GPIOF
#define M_IN2_Pin GPIO_PIN_12
#define M_IN2_GPIO_Port GPIOF
#define M_IN3_Pin GPIO_PIN_13
#define M_IN3_GPIO_Port GPIOF
#define M_IN4_Pin GPIO_PIN_14
#define M_IN4_GPIO_Port GPIOF
#define M_IN5_Pin GPIO_PIN_15
#define M_IN5_GPIO_Port GPIOF
#define M_IN6_Pin GPIO_PIN_0
#define M_IN6_GPIO_Port GPIOG
#define M_IN7_Pin GPIO_PIN_1
#define M_IN7_GPIO_Port GPIOG
#define M_IN8_Pin GPIO_PIN_7
#define M_IN8_GPIO_Port GPIOE
#define M_IN9_Pin GPIO_PIN_8
#define M_IN9_GPIO_Port GPIOE
#define M_IN10_Pin GPIO_PIN_9
#define M_IN10_GPIO_Port GPIOE
#define M_IN11_Pin GPIO_PIN_10
#define M_IN11_GPIO_Port GPIOE
#define M_IN12_Pin GPIO_PIN_11
#define M_IN12_GPIO_Port GPIOE
#define M_IN13_Pin GPIO_PIN_12
#define M_IN13_GPIO_Port GPIOE
#define M_IN14_Pin GPIO_PIN_13
#define M_IN14_GPIO_Port GPIOE
#define M_IN15_Pin GPIO_PIN_14
#define M_IN15_GPIO_Port GPIOE
#define M_IN16_Pin GPIO_PIN_15
#define M_IN16_GPIO_Port GPIOE
#define M_RL9_Pin GPIO_PIN_8
#define M_RL9_GPIO_Port GPIOB
#define M_RL10_Pin GPIO_PIN_9
#define M_RL10_GPIO_Port GPIOB
#define M_RL11_Pin GPIO_PIN_0
#define M_RL11_GPIO_Port GPIOE
#define M_RL12_Pin GPIO_PIN_1
#define M_RL12_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

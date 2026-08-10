/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, M_RL13_Pin|M_RL14_Pin|M_RL15_Pin|M_RL16_Pin
                          |M_RL1_Pin|M_RL11_Pin|M_RL12_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CPU_STA_GPIO_Port, CPU_STA_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, M_RL2_Pin|M_RL3_Pin|M_RL4_Pin|M_RL5_Pin
                          |M_RL6_Pin|M_RL7_Pin|M_RL8_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, M_RL9_Pin|M_RL10_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : M_RL13_Pin M_RL14_Pin M_RL15_Pin M_RL16_Pin
                           M_RL1_Pin M_RL11_Pin M_RL12_Pin */
  GPIO_InitStruct.Pin = M_RL13_Pin|M_RL14_Pin|M_RL15_Pin|M_RL16_Pin
                          |M_RL1_Pin|M_RL11_Pin|M_RL12_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : CPU_STA_Pin */
  GPIO_InitStruct.Pin = CPU_STA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CPU_STA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : M_RL2_Pin M_RL3_Pin M_RL4_Pin M_RL5_Pin
                           M_RL6_Pin M_RL7_Pin M_RL8_Pin */
  GPIO_InitStruct.Pin = M_RL2_Pin|M_RL3_Pin|M_RL4_Pin|M_RL5_Pin
                          |M_RL6_Pin|M_RL7_Pin|M_RL8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : M_IN1_Pin M_IN2_Pin M_IN3_Pin M_IN4_Pin
                           M_IN5_Pin */
  GPIO_InitStruct.Pin = M_IN1_Pin|M_IN2_Pin|M_IN3_Pin|M_IN4_Pin
                          |M_IN5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : M_IN6_Pin M_IN7_Pin */
  GPIO_InitStruct.Pin = M_IN6_Pin|M_IN7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : M_IN8_Pin M_IN9_Pin M_IN10_Pin M_IN11_Pin
                           M_IN12_Pin M_IN13_Pin M_IN14_Pin M_IN15_Pin
                           M_IN16_Pin */
  GPIO_InitStruct.Pin = M_IN8_Pin|M_IN9_Pin|M_IN10_Pin|M_IN11_Pin
                          |M_IN12_Pin|M_IN13_Pin|M_IN14_Pin|M_IN15_Pin
                          |M_IN16_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : M_RL9_Pin M_RL10_Pin */
  GPIO_InitStruct.Pin = M_RL9_Pin|M_RL10_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

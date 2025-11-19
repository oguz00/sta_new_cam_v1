/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */
#define VEHICLE_TX_Pin GPIO_PIN_2
#define VEHICLE_TX_GPIO_Port GPIOA
#define VEHICLE_RX_Pin GPIO_PIN_3
#define VEHICLE_RX_GPIO_Port GPIOA
#define CAM_TX_Pin GPIO_PIN_9
#define CAM_TX_GPIO_Port GPIOA
#define CAM_RX_Pin GPIO_PIN_10
#define CAM_RX_GPIO_Port GPIOA

#define EMPTY_Pin GPIO_PIN_6
#define EMPTY_GPIO_Port GPIOA
#define PALETTE_CTL_Pin GPIO_PIN_0
#define PALETTE_CTL_GPIO_Port GPIOB
#define ZOOM_CTL_Pin GPIO_PIN_7
#define ZOOM_CTL_GPIO_Port GPIOA
#define SHUTTER_CTL_Pin GPIO_PIN_5
#define SHUTTER_CTL_GPIO_Port GPIOA
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

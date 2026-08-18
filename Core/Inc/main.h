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
#include "stm32f1xx_hal.h"

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
#define BA3_Pin GPIO_PIN_2
#define BA3_GPIO_Port GPIOE
#define BA4_Pin GPIO_PIN_3
#define BA4_GPIO_Port GPIOE
#define BE1_Pin GPIO_PIN_2
#define BE1_GPIO_Port GPIOA
#define BE2_Pin GPIO_PIN_3
#define BE2_GPIO_Port GPIOA
#define BE3_Pin GPIO_PIN_4
#define BE3_GPIO_Port GPIOA
#define BE4_Pin GPIO_PIN_5
#define BE4_GPIO_Port GPIOA
#define BE5_Pin GPIO_PIN_6
#define BE5_GPIO_Port GPIOA
#define BE6_Pin GPIO_PIN_7
#define BE6_GPIO_Port GPIOA
#define UPR_Down_Pin GPIO_PIN_8
#define UPR_Down_GPIO_Port GPIOE
#define Sync_Pin GPIO_PIN_10
#define Sync_GPIO_Port GPIOE
#define Dig_1_Pin GPIO_PIN_10
#define Dig_1_GPIO_Port GPIOB
#define Dig_2_Pin GPIO_PIN_11
#define Dig_2_GPIO_Port GPIOB
#define Dig_3_Pin GPIO_PIN_12
#define Dig_3_GPIO_Port GPIOB
#define Dig_4_Pin GPIO_PIN_13
#define Dig_4_GPIO_Port GPIOB
#define Dig_5_Pin GPIO_PIN_14
#define Dig_5_GPIO_Port GPIOB
#define Dig_6_Pin GPIO_PIN_15
#define Dig_6_GPIO_Port GPIOB
#define Seg_A_Pin GPIO_PIN_8
#define Seg_A_GPIO_Port GPIOD
#define Seg_B_Pin GPIO_PIN_9
#define Seg_B_GPIO_Port GPIOD
#define Seg_C_Pin GPIO_PIN_10
#define Seg_C_GPIO_Port GPIOD
#define Seg_D_Pin GPIO_PIN_11
#define Seg_D_GPIO_Port GPIOD
#define Seg_E_Pin GPIO_PIN_12
#define Seg_E_GPIO_Port GPIOD
#define Seg_F_Pin GPIO_PIN_13
#define Seg_F_GPIO_Port GPIOD
#define Seg_G_Pin GPIO_PIN_14
#define Seg_G_GPIO_Port GPIOD
#define Seg_DP_Pin GPIO_PIN_15
#define Seg_DP_GPIO_Port GPIOD
#define BA1_Pin GPIO_PIN_0
#define BA1_GPIO_Port GPIOE
#define BA2_Pin GPIO_PIN_1
#define BA2_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

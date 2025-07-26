/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#define USB_VBUS_Pin GPIO_PIN_2
#define USB_VBUS_GPIO_Port GPIOE
#define BLUE_PUSH_BUTT_Pin GPIO_PIN_13
#define BLUE_PUSH_BUTT_GPIO_Port GPIOC
#define PH0_OSC_IN_Pin GPIO_PIN_0
#define PH0_OSC_IN_GPIO_Port GPIOH
#define PH1_OSC_OUT_Pin GPIO_PIN_1
#define PH1_OSC_OUT_GPIO_Port GPIOH
#define HM_ACS712_ADC_Pin GPIO_PIN_0
#define HM_ACS712_ADC_GPIO_Port GPIOA
#define VR1_ADCIN1_Pin GPIO_PIN_1
#define VR1_ADCIN1_GPIO_Port GPIOA
#define VR2_ADCIN2_Pin GPIO_PIN_2
#define VR2_ADCIN2_GPIO_Port GPIOA
#define VC_ADCIN4_Pin GPIO_PIN_3
#define VC_ADCIN4_GPIO_Port GPIOA
#define AC60HZ_S_Pin GPIO_PIN_4
#define AC60HZ_S_GPIO_Port GPIOA
#define HM_OPA_ADC_Pin GPIO_PIN_5
#define HM_OPA_ADC_GPIO_Port GPIOA
#define BD_Pin GPIO_PIN_7
#define BD_GPIO_Port GPIOA
#define USB_EN_Pin GPIO_PIN_14
#define USB_EN_GPIO_Port GPIOB
#define VACUUM_CTRL_Pin GPIO_PIN_8
#define VACUUM_CTRL_GPIO_Port GPIOA
#define HM_DO_Pin GPIO_PIN_9
#define HM_DO_GPIO_Port GPIOA
#define OTG_FS_DM_Pin GPIO_PIN_11
#define OTG_FS_DM_GPIO_Port GPIOA
#define OTG_FS_DP_Pin GPIO_PIN_12
#define OTG_FS_DP_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define I2C1_INT_Pin GPIO_PIN_5
#define I2C1_INT_GPIO_Port GPIOB
#define S2_Pin GPIO_PIN_8
#define S2_GPIO_Port GPIOB
#define S1_Pin GPIO_PIN_9
#define S1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define DEBUG_UART_PORT &huart6
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

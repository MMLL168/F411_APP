/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "acs712.h"
#include "current_monitor.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "handpiece.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// 宣告 ACS712 和電流監控變數
//ACS712_Type_t acs712;
ACS712_Handle_t acs712;
Current_Monitor_t monitor;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len)
{
    // 使用 10ms 超時，失敗也返回成功
    HAL_UART_Transmit(DEBUG_UART_PORT, (uint8_t*)ptr, len, 10);
    return len;  // 總是返回成功
}

void Test_Current_Monitor_With_Filters(void);
void Menu_Selection(void);
void Test_Current_Monitor_With_Filters(void)
{
    // 初始化電流監控系統
    Current_Monitor_t monitor;
    ACS712_Handle_t acs712;

    // 初始化 ACS712
    ACS712_Init(&acs712, &hadc1, ACS712_05A);

    // 初始化電流監控器
    CurrentMonitor_Init(&monitor, &acs712);

    printf("\r\n=== 電流監控濾波器測試系統 ===\r\n");
    printf("初始化完成，開始測試...\r\n\r\n");

    // **執行濾波器測試**
    CurrentMonitor_TestFilters(&monitor);

    printf("\r\n=== 進入持續監控模式 ===\r\n");

    // **持續監控模式**
    while (1) {
        CurrentMonitor_Update(&monitor);

        // 每5秒顯示一次詳細資訊
        static uint32_t last_display = 0;
        if (HAL_GetTick() - last_display > 5000) {
            CurrentMonitor_Display(&monitor);
            last_display = HAL_GetTick();
        }

        HAL_Delay(100);
    }
}

/**
 * @brief  選單系統
 * @param  None
 * @retval None
 */
void Menu_Selection(void)
{
    printf("\r\n=== 測試選單 ===\r\n");
    printf("1. PWM 測試\r\n");
    printf("2. ADC 測試\r\n");
    printf("3. 電流監控濾波器測試\r\n");
    printf("4. 電流監控基本測試\r\n");
    printf("請選擇測試項目 (1-4):\r\n");

    // 這裡可以加入 UART 接收選擇邏輯
    // 暫時直接執行濾波器測試
    Test_Current_Monitor_With_Filters();
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
    // 設置向量表到APP區域
  SCB->VTOR = 0x08008000;
  __disable_irq();
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  __enable_irq();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  MX_TIM4_Init();
  MX_USB_DEVICE_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
  // 🔧 簡化測試：先測試基本功能
  printf("=== APP STARTED ===\r\n");

  // 測試 LED（PA5 是板載 LED）
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = HM_OPA_ADC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(HM_OPA_ADC_GPIO_Port, &GPIO_InitStruct);
  HAL_GPIO_WritePin(HM_OPA_ADC_GPIO_Port, HM_OPA_ADC_Pin, GPIO_PIN_SET);
  HAL_Delay(500);
  HAL_GPIO_WritePin(HM_OPA_ADC_GPIO_Port, HM_OPA_ADC_Pin, GPIO_PIN_RESET);

  printf("LED Test completed\r\n");

  /* 初始化SSD1306 */
  char buf[32];
  printf("ssd1306_Init...\r\n");
  ssd1306_Init();

  ssd1306_Fill(Black);
  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString("ACS712_Init...", Font_6x8, White);
  ssd1306_UpdateScreen();

  /* 初始化ACS712 */
  printf("ACS712_Init...\r\n");
  if (ACS712_Init(&acs712, &hadc1, ACS712_05A) != HAL_OK)
  {
	  printf("ACS712_Init Fail!!!\r\n");
      Error_Handler();
  }


  /* 校準ACS712 */
  //ssd1306_Fill(Black);
  ssd1306_SetCursor(0, 8);
  ssd1306_WriteString("Calibrating...", Font_6x8, White);
  ssd1306_UpdateScreen();

  printf("ACS712 Calibrating ...ADC to OFFSET\r\n");
  if (ACS712_Calibrate(&acs712) != HAL_OK)
  {
	  printf("ACS712 Calibrating FAIL!!!\r\n");
      Error_Handler();
  }

  /* 初始化電流監控器 */
  ssd1306_SetCursor(0, 16);
  ssd1306_WriteString("CurrentMonitor_Init...", Font_6x8, White);
  ssd1306_UpdateScreen();
  printf("CurrentMonitor_Init...\r\n");
  if (CurrentMonitor_Init(&monitor, &acs712) != HAL_OK)
  {
	  printf("CurrentMonitor_Init Fail!!!\r\n");
      Error_Handler();
  }
  HAL_Delay(300);


  printf("Zero offset: %.3f V\r\n", acs712.zero_offset);

  sprintf(buf, "Zero offset: %.3fV", acs712.zero_offset);
  ssd1306_Fill(Black);
  ssd1306_SetCursor(0, 0); // 設定顯示位置
  ssd1306_WriteString(buf, Font_6x8, White);
  ssd1306_UpdateScreen();
  HAL_Delay(100);

  // 執行校準
  CurrentMonitor_ManualCalibration(&monitor);

  CurrentMonitor_ResetStats(&monitor);

  HAL_Delay(100);

  while(1)
  {
  	CurrentMonitor_TestFilters(&monitor);


  }

  /* 主迴圈 */
  ssd1306_SetCursor(0, 16);
  ssd1306_WriteString("CurrentMonitor Start  ...", Font_6x8, White);
  ssd1306_UpdateScreen();
  printf("CurrentMonitor Start  ...\r\n");
  while (1)
  {
      CurrentMonitor_Update(&monitor);
      CurrentMonitor_Display(&monitor);
      CheckAutoReset(&monitor);  // 加入這行

      HAL_Delay(500);
  }


  uint32_t counter = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // 簡單的 LED 閃爍測試
      HAL_GPIO_TogglePin(HM_OPA_ADC_GPIO_Port, HM_OPA_ADC_Pin);

      // 簡化的串口輸出
      printf("APP Running: %lu\r\n", counter++);

      //TestADC();
      TestADC_Averaging();
      HAL_Delay(1000);  // 改為 1 秒，更容易觀察
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* USER CODE BEGIN */
  // 🔧 先重置到默認狀態
  HAL_RCC_DeInit();
  /* USER CODE END */

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

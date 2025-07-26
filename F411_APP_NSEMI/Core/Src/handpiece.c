/*
 * handpiece.c
 *
 *  Created on: Jul 24, 2025
 *      Author: User
 */
#include "main.h"        // ✅ 需要這個來取得 htim1
#include "tim.h"         // ✅ 或者這個，看你的專案結構
#include "adc.h"
#include "handpiece.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
//=========================================PWM============================================//
void Start_PWM(void)
{
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

void Stop_PWM(void)
{
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
}

void Set_PWM_DutyCycle(uint16_t duty_cycle)
{
    // duty_cycle 範圍：0 ~ 999 (對應 0% ~ 100%)
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty_cycle);
}

void Set_PWM_Frequency(uint32_t frequency_hz)
{
    uint32_t timer_clock;
    uint32_t prescaler = 72; // 保持原有預分頻器
    uint32_t period;

    // 根據你的系統時鐘調整
    timer_clock = HAL_RCC_GetPCLK2Freq(); // TIM1 在 APB2 上

    // 計算週期值
    period = (timer_clock / prescaler / frequency_hz) - 1;

    // 停止 PWM
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);

    // 更新週期
    __HAL_TIM_SET_AUTORELOAD(&htim1, period);

    // 重新啟動 PWM
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

void TestDuty(void)
{
  int16_t duty = 0;

  // 範例：動態調整佔空比
  while (1)
  {
	  Start_PWM();

	  for( duty = 0; duty <= 999; duty += 50)
	  {
		  Set_PWM_DutyCycle(duty);
		  HAL_Delay(100);
	  }

	  for( duty = 999; duty > 0; duty -= 50)
	  {
		  Set_PWM_DutyCycle(duty);
		  HAL_Delay(100);
	  }
	  Stop_PWM();

      HAL_Delay(100);
  }
}

//=========================================ADC============================================//
extern ADC_HandleTypeDef hadc1;

volatile uint16_t adc_value = 0;
volatile float voltage = 0.0f;
extern volatile uint8_t adc_updated;

void Start_ADC_Sampling(void)
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_value, 1);
}

void Stop_ADC_Sampling(void)
{
    HAL_ADC_Stop_DMA(&hadc1);
}

void Start_ADC_DMA(void)
{
    // 單次讀取模式
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_value, 1);

    // 或者多次採樣平均模式
    // HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, ADC_BUFFER_SIZE);
}


void TestADC(void)
{
    // 啟動連續 ADC DMA 採樣
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_value, 1);

    while (1)
    {
        if(adc_updated)
        {
            printf("ADC Value: %d, Voltage: %.3f V\r\n", adc_value, voltage);
            adc_updated = 0;
            HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_value, 1);
        }
        HAL_Delay(100);
    }
}

// ADC DMA 緩衝區
// 如果需要多次採樣平均
#define ADC_BUFFER_SIZE 100
#define ADC_SAMPLES 10
volatile uint8_t adc_complete = 0;
volatile uint16_t adc_buffer[ADC_BUFFER_SIZE];
volatile uint32_t adc_sum = 0;
volatile uint16_t adc_average = 0;

void TestADC_Averaging(void)
{
    while (1)
    {
        // 啟動多次採樣
        adc_complete = 0;
        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, ADC_SAMPLES);

        // 等待採樣完成
        while(!adc_complete);

        // 計算平均值
        uint32_t sum = 0;
        for(int i = 0; i < ADC_SAMPLES; i++)
        {
            sum += adc_buffer[i];
        }
        uint16_t average = sum / ADC_SAMPLES;
        float avg_voltage = (float)average * 3.3f / 4095.0f;

        printf("ADC Average: %d, Voltage: %.3f V\r\n", average, avg_voltage);

        HAL_Delay(100);
    }
}

// 在 handpiece.c 中加入
float ADC_To_Voltage(uint16_t adc_val)
{
    return (float)adc_val * 3.3f / 4095.0f;
}

uint8_t ADC_To_Percentage(uint16_t adc_val)
{
    return (uint8_t)((adc_val * 100) / 4095);
}


// 濾波器類型
typedef enum {
    FILTER_NONE = 0,
    FILTER_MOVING_AVERAGE,
    FILTER_KALMAN,
    FILTER_BOTH
} FilterType_t;

// 濾波後的數據
volatile uint16_t adc_filtered_ma[ADC_CHANNEL_COUNT];     // 移動平均濾波
volatile uint16_t adc_filtered_kalman[ADC_CHANNEL_COUNT]; // 卡爾曼濾波

// 移動平均濾波
#define FILTER_SIZE 10
static uint16_t filter_buffer[ADC_CHANNEL_COUNT][FILTER_SIZE] = {0};
static uint8_t filter_index[ADC_CHANNEL_COUNT] = {0};

// 卡爾曼濾波器（每個通道獨立）
static KalmanFilter_t kalman_filters[ADC_CHANNEL_COUNT];

// 初始化所有濾波器
void ADC_Filter_Init(void)
{
    // 初始化移動平均濾波器
    for(int ch = 0; ch < ADC_CHANNEL_COUNT; ch++)
    {
        filter_index[ch] = 0;
        for(int i = 0; i < FILTER_SIZE; i++)
        {
            filter_buffer[ch][i] = 0;
        }
    }

    // 初始化卡爾曼濾波器
    for(int ch = 0; ch < ADC_CHANNEL_COUNT; ch++)
    {
        Kalman_Init(&kalman_filters[ch],
                   2048.0f,    // 初始值（12位元 ADC 中點）
                   1.0f,       // 過程噪聲（可調整）
                   25.0f);     // 測量噪聲（可調整）
    }
}

// 移動平均濾波（每個通道獨立）
uint16_t ADC_MovingAverage(uint16_t new_value, uint8_t channel)
{
    if(channel >= ADC_CHANNEL_COUNT) return new_value;

    filter_buffer[channel][filter_index[channel]] = new_value;
    filter_index[channel] = (filter_index[channel] + 1) % FILTER_SIZE;

    uint32_t sum = 0;
    for(int i = 0; i < FILTER_SIZE; i++)
    {
        sum += filter_buffer[channel][i];
    }

    return sum / FILTER_SIZE;
}

// 卡爾曼濾波器初始化
void Kalman_Init(KalmanFilter_t* kf, float initial_value, float process_noise, float measurement_noise)
{
    kf->x = initial_value;          // 初始狀態估計
    kf->P = 1.0f;                   // 初始估計誤差協方差
    kf->Q = process_noise;          // 過程噪聲協方差
    kf->R = measurement_noise;      // 測量噪聲協方差
    kf->K = 0.0f;                   // 卡爾曼增益
    kf->initialized = 1;
}

// 卡爾曼濾波器更新
float Kalman_Update(KalmanFilter_t* kf, float measurement)
{
    if(!kf->initialized) return measurement;

    // 預測步驟
    // x_pred = x (假設狀態轉移矩陣為1)
    // P_pred = P + Q
    float P_pred = kf->P + kf->Q;

    // 更新步驟
    // K = P_pred / (P_pred + R)
    kf->K = P_pred / (P_pred + kf->R);

    // x = x_pred + K * (measurement - x_pred)
    kf->x = kf->x + kf->K * (measurement - kf->x);

    // P = (1 - K) * P_pred
    kf->P = (1.0f - kf->K) * P_pred;

    return kf->x;
}

// 卡爾曼濾波（每個通道獨立）
uint16_t ADC_KalmanFilter(uint16_t new_value, uint8_t channel)
{
    if(channel >= ADC_CHANNEL_COUNT) return new_value;

    float filtered_value = Kalman_Update(&kalman_filters[channel], (float)new_value);

    // 限制範圍在 ADC 有效範圍內
    if(filtered_value < 0) filtered_value = 0;
    if(filtered_value > 4095) filtered_value = 4095;

    return (uint16_t)filtered_value;
}

// 處理所有濾波器
void ADC_Process_All_Filters(uint8_t channel, uint16_t raw_value)
{
    if(channel >= ADC_CHANNEL_COUNT) return;

    // 移動平均濾波
    adc_filtered_ma[channel] = ADC_MovingAverage(raw_value, channel);
    // 卡爾曼濾波
    adc_filtered_kalman[channel] = ADC_KalmanFilter(raw_value, channel);
}

// 設定卡爾曼濾波器參數
void Kalman_Set_Parameters(uint8_t channel, float process_noise, float measurement_noise)
{
    if(channel >= ADC_CHANNEL_COUNT) return;

    kalman_filters[channel].Q = process_noise;
    kalman_filters[channel].R = measurement_noise;
}


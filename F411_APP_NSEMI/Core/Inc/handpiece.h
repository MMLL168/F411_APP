/*
 * handpiece.h
 *
 *  Created on: Jul 24, 2025
 *      Author: User
 */

#ifndef INC_HANDPIECE_H_
#define INC_HANDPIECE_H_

// 卡爾曼濾波器結構體
typedef struct {
    float x;          // 狀態估計值
    float P;          // 估計誤差協方差
    float Q;          // 過程噪聲協方差
    float R;          // 測量噪聲協方差
    float K;          // 卡爾曼增益
    uint8_t initialized; // 初始化標誌
} KalmanFilter_t;

#define ADC_CHANNEL_COUNT 6

// 濾波器配置
#define FILTER_SIZE 10



void Start_PWM(void);
void Stop_PWM(void);
void Set_PWM_DutyCycle(uint16_t duty_cycle);
void Set_PWM_Frequency(uint32_t frequency_hz);
uint16_t Get_ADC_Value(void);
float Get_ADC_Voltage(void);
void Start_ADC_Sampling(void);
void Stop_ADC_Sampling(void);
void TestADC_Averaging(void);

// 濾波器函數
void ADC_Filter_Init(void);
uint16_t ADC_MovingAverage(uint16_t new_value, uint8_t channel);
uint16_t ADC_KalmanFilter(uint16_t new_value, uint8_t channel);
void ADC_Process_All_Filters(uint8_t channel, uint16_t raw_value);

// 濾波後數據讀取函數
uint16_t Get_Channel_ADC_Filtered_MA(uint8_t channel);
uint16_t Get_Channel_ADC_Filtered_Kalman(uint8_t channel);
uint32_t Get_Channel_Voltage_Filtered_MA(uint8_t channel);
uint32_t Get_Channel_Voltage_Filtered_Kalman(uint8_t channel);

// 卡爾曼濾波器配置函數
void Kalman_Init(KalmanFilter_t* kf, float initial_value, float process_noise, float measurement_noise);
float Kalman_Update(KalmanFilter_t* kf, float measurement);
void Kalman_Set_Parameters(uint8_t channel, float process_noise, float measurement_noise);
#endif /* INC_HANDPIECE_H_ */

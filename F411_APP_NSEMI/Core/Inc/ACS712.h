#ifndef __ACS712_H
#define __ACS712_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <math.h>

/* ACS712 型號定義 */
typedef enum {
    ACS712_05A = 0,  // ±5A, 185mV/A
    ACS712_20A = 1,  // ±20A, 100mV/A
    ACS712_30A = 2   // ±30A, 66mV/A
} ACS712_Type_t;

/* ACS712 配置結構 */
typedef struct {
    ADC_HandleTypeDef *hadc;
    ACS712_Type_t type;
    float sensitivity;      // mV/A
    float zero_offset;      // V
    uint32_t adc_channel;
    float vref;            // ADC參考電壓
    uint16_t adc_resolution; // ADC解析度
} ACS712_Handle_t;

/* 電流統計結構 */
typedef struct {
    float max_current;
    float min_current;
    float rms_current;
    uint32_t timestamp;
    uint32_t sample_count;
} Current_Stats_t;

/* 函數宣告 */
HAL_StatusTypeDef ACS712_Init(ACS712_Handle_t *hacs712, ADC_HandleTypeDef *hadc, ACS712_Type_t type);
HAL_StatusTypeDef ACS712_Calibrate(ACS712_Handle_t *hacs712);
float ACS712_ReadCurrent(ACS712_Handle_t *hacs712);
float ACS712_ReadCurrentFiltered(ACS712_Handle_t *hacs712, uint8_t samples);
float ACS712_CalculateRMS(ACS712_Handle_t *hacs712, uint16_t samples, uint16_t interval_ms);
void ACS712_UpdateStats(float current, Current_Stats_t *stats);
void ACS712_ResetStats(Current_Stats_t *stats);
void ACS712_InitStats(Current_Stats_t *stats);
void ACS712_ResetRMSBuffer(void);

#ifdef __cplusplus
}
#endif

#endif /* __ACS712_H */

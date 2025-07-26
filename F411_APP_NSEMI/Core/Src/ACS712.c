#include "acs712.h"
#include "current_monitor.h"

/* 私有變數 */
static float sensitivity_table[] = {0.185f, 0.100f, 0.066f}; // mV/A for 5A, 20A, 30A

/* 私有函數 */
static uint32_t ACS712_ReadADC(ACS712_Handle_t *hacs712);

/**
 * @brief  初始化ACS712
 * @param  hacs712: ACS712控制結構指標
 * @param  hadc: ADC控制結構指標
 * @param  type: ACS712型號
 * @retval HAL狀態
 */
HAL_StatusTypeDef ACS712_Init(ACS712_Handle_t *hacs712, ADC_HandleTypeDef *hadc, ACS712_Type_t type)
{
    if (hacs712 == NULL || hadc == NULL)
        return HAL_ERROR;

    hacs712->hadc = hadc;
    hacs712->type = type;
    hacs712->sensitivity = sensitivity_table[type];
    //hacs712->zero_offset = 2.5f; // 預設值，需要校準
    hacs712->zero_offset = 2.384f;  // 根據你的 ADC 讀數
    hacs712->vref = 3.3f;
    hacs712->adc_resolution = 4096;
    hacs712->adc_channel = ADC_CHANNEL_0;

    return HAL_OK;
}

/**
 * @brief  校準ACS712零點
 * @param  hacs712: ACS712控制結構指標
 * @retval HAL狀態
 */
HAL_StatusTypeDef ACS712_Calibrate(ACS712_Handle_t *hacs712)
{
    if (hacs712 == NULL)
        return HAL_ERROR;

    uint32_t sum = 0;
    const uint16_t samples = 100;

    for (uint16_t i = 0; i < samples; i++)
    {
        sum += ACS712_ReadADC(hacs712);
        HAL_Delay(10);
    }

    float average_adc = (float)sum / samples;
    hacs712->zero_offset = (average_adc * hacs712->vref) / hacs712->adc_resolution;

    return HAL_OK;
}

/**
 * @brief  讀取電流值
 * @param  hacs712: ACS712控制結構指標
 * @retval 電流值 (A)
 */
float ACS712_ReadCurrent(ACS712_Handle_t *hacs712)
{
    if (hacs712 == NULL)
        return 0.0f;

    uint32_t adc_value = ACS712_ReadADC(hacs712);
    float voltage = (adc_value * hacs712->vref) / hacs712->adc_resolution;
    float current = (voltage - hacs712->zero_offset) / hacs712->sensitivity;

    return current;
}

/**
 * @brief  讀取濾波後的電流值
 * @param  hacs712: ACS712控制結構指標
 * @param  samples: 採樣次數
 * @retval 濾波後的電流值 (A)
 */
float ACS712_ReadCurrentFiltered(ACS712_Handle_t *hacs712, uint8_t samples)
{
    if (hacs712 == NULL || samples == 0)
        return 0.0f;

    float sum = 0.0f;

    for (uint8_t i = 0; i < samples; i++)
    {
        sum += ACS712_ReadCurrent(hacs712);
        HAL_Delay(1);
    }

    return sum / samples;
}

/**
 * @brief  計算RMS電流值
 * @param  hacs712: ACS712控制結構指標
 * @param  samples: 採樣次數
 * @param  interval_ms: 採樣間隔 (ms)
 * @retval RMS電流值 (A)
 */
float ACS712_CalculateRMS(ACS712_Handle_t *hacs712, uint16_t samples, uint16_t interval_ms)
{
    if (hacs712 == NULL || samples == 0)
        return 0.0f;

    float sum_squares = 0.0f;

    for (uint16_t i = 0; i < samples; i++)
    {
        float current = ACS712_ReadCurrent(hacs712);
        sum_squares += current * current;
        HAL_Delay(interval_ms);
    }

    return sqrtf(sum_squares / samples);
}

/**
 * @brief  更新電流統計
 * @param  current: 當前電流值
 * @param  stats: 統計結構指標
 * @retval None
 */
// 在 acs712.c 中實作或修正
void ACS712_UpdateStats(float current, Current_Stats_t *stats)
{
    if (stats == NULL)
        return;

    // **第一步：對輸入電流應用死區**
    if (fabs(current) < CURRENT_DEADBAND) {
        current = 0.0f;  // 強制設為零
    }

    // 更新樣本計數
    stats->sample_count++;

    // 更新最大值（只有非零值才更新）
    if (current != 0.0f) {
        if (current > stats->max_current || stats->sample_count == 1) {
            stats->max_current = current;
        }

        // 更新最小值
        if (current < stats->min_current || stats->sample_count == 1) {
            stats->min_current = current;
        }
    }

    // **修正 RMS 計算 - 關鍵修改**
    static float rms_buffer[20] = {0};
    static int rms_index = 0;
    static int rms_count = 0;

    // **重要：存儲處理後的電流值（已應用死區）**
    rms_buffer[rms_index] = current * current;  // current 已經是處理後的值
    rms_index = (rms_index + 1) % 20;

    if (rms_count < 20) {
        rms_count++;
    }

    // 計算 RMS
    float sum_squares = 0.0f;
    for (int i = 0; i < rms_count; i++) {
        sum_squares += rms_buffer[i];
    }

    stats->rms_current = sqrtf(sum_squares / rms_count);

    // **再次應用死區到 RMS 結果**
    if (stats->rms_current < CURRENT_DEADBAND) {
        stats->rms_current = 0.0f;
    }

    stats->timestamp = HAL_GetTick();

    // **調試輸出**
    static uint32_t debug_count = 0;
    debug_count++;
    if (debug_count % 50 == 0) {  // 每50次輸出一次調試信息
        printf("DEBUG: Raw=%.1f, Processed=%.1f, RMS=%.1f mA\r\n",
               current * 1000.0f, current * 1000.0f, stats->rms_current * 1000.0f);
    }
}



// 在 acs712.c 中加入這些函數

void ACS712_InitStats(Current_Stats_t *stats)
{
    if (stats == NULL)
        return;

    stats->max_current = -999.0f;
    stats->min_current = 999.0f;
    stats->rms_current = 0.0f;
    stats->sample_count = 0;
}

void ACS712_ResetStats(Current_Stats_t *stats)
{
    ACS712_InitStats(stats);  // 重用初始化函數
}


/**
 * @brief  讀取ADC值
 * @param  hacs712: ACS712控制結構指標
 * @retval ADC原始值
 */
static uint32_t ACS712_ReadADC(ACS712_Handle_t *hacs712)
{
    HAL_ADC_Start(hacs712->hadc);
    HAL_ADC_PollForConversion(hacs712->hadc, HAL_MAX_DELAY);
    uint32_t adc_value = HAL_ADC_GetValue(hacs712->hadc);
    HAL_ADC_Stop(hacs712->hadc);

    return adc_value;
}

void ACS712_ResetRMSBuffer(void)
{
    static float rms_buffer[20] = {0};
    static int rms_index = 0;
    static int rms_count = 0;

    // 清零緩衝區
    for (int i = 0; i < 20; i++) {
        rms_buffer[i] = 0.0f;
    }
    rms_index = 0;
    rms_count = 0;
}


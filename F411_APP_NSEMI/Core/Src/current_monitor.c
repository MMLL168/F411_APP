#include "current_monitor.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>    // ← 加入這行
#include "handpiece.h"

// 本地統計初始化函數
static void InitStats(Current_Stats_t *stats)  // ← 使用 Current_Stats_t
{
    if (stats == NULL)
        return;

    stats->max_current = -999.0f;
    stats->min_current = 999.0f;
    stats->rms_current = 0.0f;
    stats->sample_count = 0;
    stats->timestamp = HAL_GetTick();  // 加入時間戳
}

/**
 * @brief  初始化電流監控器
 * @param  monitor: 監控器結構指標
 * @param  acs712: ACS712控制結構指標
 * @retval HAL狀態
 */
HAL_StatusTypeDef CurrentMonitor_Init(Current_Monitor_t *monitor, ACS712_Handle_t *acs712)
{
    if (monitor == NULL || acs712 == NULL)
        return HAL_ERROR;

    monitor->acs712 = acs712;
    monitor->status = MONITOR_NORMAL;
    monitor->voltage = 5.0f;
    monitor->power = 0.0f;
    monitor->energy_wh = 0;
    monitor->energy_start_time = HAL_GetTick();
    monitor->filter_index = 0;
    monitor->last_update = 0;
    monitor->current_now = 0.0f;  // 初始化當前電流

    // 初始化濾波器緩衝區
    memset(monitor->filter_buffer, 0, sizeof(monitor->filter_buffer));

    // 重置統計
    ACS712_ResetStats(&monitor->stats);

    return HAL_OK;
}

// 修正重置函數，確保完全清除
void CurrentMonitor_ResetStats(Current_Monitor_t *monitor)
{
    if (monitor == NULL || &monitor->stats == NULL)
        return;

    printf("重置統計數據...\r\n");

    // 重置統計數據
    monitor->stats.max_current = 0.0f;
    monitor->stats.min_current = 0.0f;
    monitor->stats.rms_current = 0.0f;
    monitor->stats.sample_count = 0;
    monitor->stats.timestamp = HAL_GetTick();

    // **清除 RMS 計算緩衝區**
    extern float rms_buffer[20];  // 如果是靜態的，需要另想辦法
    extern int rms_index;
    extern int rms_count;

    // 重置 RMS 緩衝區 - 在 ACS712_UpdateStats 中加入重置函數
    ACS712_ResetRMSBuffer();

    // 重置移動平均
    CurrentMonitor_ResetMovingAverage(monitor);

    printf("統計數據已重置\r\n");
}

void CurrentMonitor_ResetMovingAverage(Current_Monitor_t *monitor)
{
    if (monitor == NULL)
        return;

    // 重置移動平均相關的靜態變數
    // 這裡需要重置 CurrentMonitor_MovingAverage 函數中的靜態變數

    // 由於靜態變數在函數內部，我們需要通過特殊方式重置
    // 方法1: 呼叫一次移動平均函數來重置
    for (int i = 0; i < 10; i++) {
        CurrentMonitor_MovingAverage(monitor, 0.0f);
    }

    printf("移動平均已重置\r\n");
}

/**
 * @brief  更新監控器數據
 * @param  monitor: 監控器結構指標
 * @retval None
 */
// 修正 CurrentMonitor_Update 函數
void CurrentMonitor_Update(Current_Monitor_t *monitor)
{
    if (monitor == NULL)
        return;

    uint32_t now = HAL_GetTick();
    if (now - monitor->last_update < UPDATE_INTERVAL_MS)
        return;

    // **多樣本平均 + 強化死區處理**
    float current_sum = 0.0f;
    const int samples = 10;

    for (int i = 0; i < samples; i++) {
        float raw_current = ACS712_ReadCurrent(monitor->acs712);

        // **每個樣本都先應用死區**
        if (fabs(raw_current) < CURRENT_DEADBAND) {
            raw_current = 0.0f;
        }

        current_sum += raw_current;
        HAL_Delay(2);
    }

    float current_avg = current_sum / samples;

    // **再次應用死區到平均值**
    if (fabs(current_avg) < CURRENT_DEADBAND) {
        current_avg = 0.0f;
    }

    // 零點偏移補償（只對非零值進行）
    static float zero_offset = 0.0f;
    static uint32_t stable_count = 0;

    if (current_avg == 0.0f) {
        stable_count++;
        if (stable_count > 50) {  // 連續50次零電流才更新偏移
            zero_offset = 0.98f * zero_offset;  // 逐漸減小偏移
            if (stable_count == 51) {
                printf("Zero offset decay: %.1f mA\r\n", zero_offset * 1000.0f);
            }
        }
    } else {
        stable_count = 0;
    }

    // 應用零點補償（只對非零值）
    float current_compensated = current_avg;
    if (current_avg != 0.0f) {
        current_compensated = current_avg - zero_offset;

        // **補償後再次檢查死區**
        if (fabs(current_compensated) < CURRENT_DEADBAND) {
            current_compensated = 0.0f;
        }
    }

    // **最終確認 - 三重死區檢查**
    if (fabs(current_compensated) < CURRENT_DEADBAND) {
        current_compensated = 0.0f;
    }

    // 更新當前值
    monitor->current_now = current_compensated;

    // 移動平均濾波
    float filtered_current = CurrentMonitor_MovingAverage(monitor, current_compensated);

    CurrentMonitor_UpdateStats(monitor, current_compensated);

    // 計算功率
    CurrentMonitor_CalculatePower(monitor, fabs(filtered_current));

    monitor->last_update = now;
}


/**
 * @brief  顯示監控數據
 * @param  monitor: 監控器結構指標
 * @retval None
 */
void CurrentMonitor_Display(Current_Monitor_t *monitor)
{
    if (monitor == NULL)
        return;

    float abs_current = fabs(monitor->current_now);
    float rms_current = monitor->stats.rms_current;

    const char* status;
    const char* status_info;
    const char* signal_status;

    // **優化的狀態判斷邏輯**
    if (abs_current == 0.0f && rms_current < FAN_5V_NOISE_THRESHOLD) {
        status = "STOPPED";
        status_info = "No Load - Clean Signal";
        signal_status = "CLEAN - No noise";
    }
    else if (rms_current >= FAN_5V_RUNNING_THRESHOLD) {
        status = "RUNNING";
        status_info = "Fan Operating";
        signal_status = "Fan running";
    }
    else if (rms_current >= FAN_5V_STARTUP_THRESHOLD) {
        status = "STARTING";
        status_info = "Fan Starting Up";
        signal_status = "Startup current";
    }
    else if (abs_current >= FAN_5V_DETECTION_THRESHOLD) {
        status = "DETECTED";
        status_info = "Load Detected";
        signal_status = "Current detected";
    }
    else if (abs_current == 0.0f && rms_current >= FAN_5V_NOISE_THRESHOLD) {
        status = "NOISE";
        status_info = "RMS Noise Present";
        signal_status = "DEADBAND working, RMS noise";
    }
    else if (abs_current > 0.0f && abs_current < FAN_5V_DETECTION_THRESHOLD) {
        status = "WEAK";
        status_info = "Weak Signal";
        signal_status = "Below detection threshold";
    }
    else {
        status = "NOISE";
        status_info = "Sensor Noise/Offset";
        signal_status = "Low level noise";
    }

    printf("\r\n=== 5V DC Fan Current Monitor ===\r\n");
    printf("Fan Status:   %s\r\n", status);
    printf("Current Now:  %.1f mA\r\n", monitor->current_now * 1000.0f);
    printf("Current Abs:  %.1f mA\r\n", abs_current * 1000.0f);
    printf("RMS Current:  %.1f mA\r\n", rms_current * 1000.0f);
    printf("Max Current:  +%.1f mA\r\n", monitor->stats.max_current * 1000.0f);
    printf("Min Current:  %.1f mA\r\n", monitor->stats.min_current * 1000.0f);
    printf("Voltage:      %.1f V\r\n", monitor->voltage);
    printf("Power:        %.0f mW\r\n", monitor->power * 1000.0f);
    printf("Sample Count: %lu\r\n", monitor->stats.sample_count);
    printf("Status Info:  %s\r\n", status_info);
    printf("Signal Status: %s: %.1f mA\r\n", signal_status,
           (abs_current > 0.0f) ? abs_current * 1000.0f : rms_current * 1000.0f);
    printf("Deadband:     %.0f mA\r\n", CURRENT_DEADBAND * 1000.0f);

    // **添加閾值參考信息**
    printf("Thresholds:   Detection=%.0f, Startup=%.0f, Running=%.0f, Noise=%.0f mA\r\n",
           FAN_5V_DETECTION_THRESHOLD * 1000.0f,
           FAN_5V_STARTUP_THRESHOLD * 1000.0f,
           FAN_5V_RUNNING_THRESHOLD * 1000.0f,
           FAN_5V_NOISE_THRESHOLD * 1000.0f);
    printf("================================\r\n\r\n");
}


// 新增手動零點校準函數
void CurrentMonitor_ManualCalibration(Current_Monitor_t *monitor)
{
    printf("=== 開始手動零點校準 ===\r\n");
    printf("請確保沒有負載連接...\r\n");

    HAL_Delay(2000);  // 等待2秒

    float sum = 0.0f;
    const int cal_samples = 100;

    printf("採集 %d 個樣本...\r\n", cal_samples);

    for (int i = 0; i < cal_samples; i++) {
        float current = ACS712_ReadCurrent(monitor->acs712);
        sum += current;

        if (i % 20 == 0) {
            printf("進度: %d/%d, 當前讀數: %.1f mA\r\n",
                   i, cal_samples, current * 1000.0f);
        }

        HAL_Delay(50);
    }

    float offset = sum / cal_samples;

    printf("計算出的零點偏移: %.1f mA\r\n", offset * 1000.0f);
    printf("標準差: ");

    // 計算標準差
    float variance_sum = 0.0f;
    for (int i = 0; i < 20; i++) {
        float current = ACS712_ReadCurrent(monitor->acs712);
        float diff = current - offset;
        variance_sum += diff * diff;
        HAL_Delay(50);
    }

    float std_dev = sqrtf(variance_sum / 20.0f);
    printf("%.1f mA\r\n", std_dev * 1000.0f);

    if (std_dev > 0.020f) {  // 標準差大於20mA
        printf("警告: 噪聲過大，建議檢查硬體連接！\r\n");
    }

    // 可以將偏移值存儲到 EEPROM 或全域變數
    // monitor->acs712->zero_offset = offset;

    printf("校準完成！\r\n");
    printf("========================\r\n\r\n");
}


/**
 * @brief  檢查過電流
 * @param  monitor: 監控器結構指標
 * @retval None
 */

void CurrentMonitor_CheckOvercurrent(Current_Monitor_t *monitor)
{
    if (monitor == NULL)
        return;

    // 計算正確的 RMS 值
    float current_rms = (monitor->stats.sample_count > 0) ?
                       sqrtf(monitor->stats.rms_current / monitor->stats.sample_count) : 0.0f;

    if (current_rms > OVERCURRENT_THRESHOLD) {
        // 過電流處理...
    }
}


/**
 * @brief  移動平均濾波
 * @param  monitor: 監控器結構指標
 * @param  new_value: 新數值
 * @retval 濾波後的數值
 */
float CurrentMonitor_MovingAverage(Current_Monitor_t *monitor, float new_value)
{
    if (monitor == NULL)
        return new_value;

    monitor->filter_buffer[monitor->filter_index] = new_value;
    monitor->filter_index = (monitor->filter_index + 1) % FILTER_SIZE;

    float sum = 0.0f;
    for (int i = 0; i < FILTER_SIZE; i++)
    {
        sum += monitor->filter_buffer[i];
    }

    return sum / FILTER_SIZE;
}

/**
 * @brief  計算功率和電能
 * @param  monitor: 監控器結構指標
 * @param  current: 電流值
 * @retval None
 */
void CurrentMonitor_CalculatePower(Current_Monitor_t *monitor, float current)
{
    if (monitor == NULL)
        return;

    // 計算功率 (假設電阻性負載)
    monitor->power = fabs(current) * monitor->voltage;

    // 計算累積電能 (Wh)
    uint32_t current_time = HAL_GetTick();
    uint32_t time_diff = current_time - monitor->energy_start_time;

    if (time_diff >= 3600000) // 每小時更新一次
    {
        float hours = (float)time_diff / 3600000.0f;
        monitor->energy_wh += (uint32_t)(monitor->power * hours);
        monitor->energy_start_time = current_time;
    }
}

/**
 * @brief  重置電能計數
 * @param  monitor: 監控器結構指標
 * @retval None
 */
void CurrentMonitor_ResetEnergy(Current_Monitor_t *monitor)
{
    if (monitor == NULL)
        return;

    monitor->energy_wh = 0;
    monitor->energy_start_time = HAL_GetTick();
}

void CheckAutoReset(Current_Monitor_t *monitor)
{
    static uint32_t last_reset = 0;
    uint32_t now = HAL_GetTick();

    // 如果檢測到持續噪聲，自動重置
    if (monitor->stats.sample_count > 100) {
        float abs_max = fabs(monitor->stats.max_current);
        float abs_min = fabs(monitor->stats.min_current);

        // 如果最大最小值都在噪聲範圍內，且 RMS 也很小
        if (abs_max < 0.100f && abs_min < 0.100f && monitor->stats.rms_current < 0.080f) {
            if (now - last_reset > 30000) {  // 30秒重置一次
                printf("檢測到低電流噪聲，自動重置統計...\r\n");
                CurrentMonitor_ResetStats(monitor);
                last_reset = now;
            }
        }
    }
}

/**
 * @brief  更新電流統計數據（帶自動重置功能）
 * @param  monitor: 監控器結構指標
 * @param  current: 當前電流值
 * @retval None
 */
void CurrentMonitor_UpdateStats(Current_Monitor_t *monitor, float current)
{
    if (monitor == NULL) return;

    // **每1000個樣本重置一次統計**
    if (monitor->stats.sample_count >= 50) {
        monitor->stats.max_current = current;
        monitor->stats.min_current = current;
        monitor->stats.sample_count = 0;
        printf("Auto-reset stats after 1000 samples\r\n");
    }

    monitor->stats.sample_count++;

    // 更新最大最小值
    if (monitor->stats.sample_count == 1) {
        // 第一個樣本
        monitor->stats.max_current = current;
        monitor->stats.min_current = current;
    } else {
        if (current > monitor->stats.max_current) {
            monitor->stats.max_current = current;
        }
        if (current < monitor->stats.min_current) {
            monitor->stats.min_current = current;
        }
    }

    // **RMS計算（簡化版本）**
    float current_squared = current * current;
    if (monitor->stats.sample_count == 1) {
        monitor->stats.rms_current = fabs(current);
    } else {
        float prev_rms_squared = monitor->stats.rms_current * monitor->stats.rms_current;
        float new_rms_squared = (prev_rms_squared * (monitor->stats.sample_count - 1) + current_squared) / monitor->stats.sample_count;
        monitor->stats.rms_current = sqrtf(new_rms_squared);
    }
}

/**
 * @brief  測試不同濾波器對電流監控的效果
 * @param  monitor: 監控器結構指標
 * @retval None
 */

extern volatile uint16_t adc_filtered_ma[ADC_CHANNEL_COUNT];     // 移動平均濾波
extern volatile uint16_t adc_filtered_kalman[ADC_CHANNEL_COUNT]; // 卡爾曼濾波


void CurrentMonitor_TestFilters(Current_Monitor_t *monitor)
{
    if (monitor == NULL) return;

    printf("=== 電流濾波器測試 ===\r\n");

    // 初始化濾波器
    ADC_Filter_Init();

    // 設定卡爾曼濾波器參數（針對電流測量優化）
    Kalman_Set_Parameters(0, 0.1f, 5.0f);  // 通道0：低過程噪聲，中等測量噪聲
    Kalman_Set_Parameters(1, 0.5f, 10.0f); // 通道1：中等過程噪聲，高測量噪聲

    for (int test_count = 0; test_count < 100; test_count++) {
        // 讀取原始電流
        float raw_current = ACS712_ReadCurrent(monitor->acs712);

        // **修正 ADC 轉換範圍**
        // 假設電流範圍是 0 到 5A，對應 ADC 0-4095
        uint16_t current_as_adc;
        if (raw_current < 0) {
            current_as_adc = 0;  // 負電流設為0
        } else if (raw_current > 5.0f) {
            current_as_adc = 4095;  // 超過5A設為最大值
        } else {
            current_as_adc = (uint16_t)(raw_current * 4095.0f / 5.0f);
        }
        // 處理所有濾波器
        ADC_Process_All_Filters(0, current_as_adc); // 通道0：卡爾曼參數1
        ADC_Process_All_Filters(1, current_as_adc); // 通道1：卡爾曼參數2

        // **修正轉換回電流值**
        float filtered_ma = ((float)adc_filtered_ma[0] * 5.0f / 4095.0f);
        float filtered_kalman1 = ((float)adc_filtered_kalman[0] * 5.0f / 4095.0f);
        float filtered_kalman2 = ((float)adc_filtered_kalman[1] * 5.0f / 4095.0f);

        // **修正噪聲減少計算**
        float noise_reduction = 0.0f;
        if (raw_current > 0.001f) {  // 避免除零
            noise_reduction = (1.0f - fabs(filtered_kalman1 - raw_current) / raw_current) * 100.0f;
        }

        // 每10次顯示一次結果
        if ((test_count % 10 == 0) && (test_count != 0)) {
            printf("Test %d:\r\n", test_count);
            printf("  Raw:      %.1f mA\r\n", raw_current * 1000.0f);
            printf("  MovAvg:   %.1f mA\r\n", filtered_ma * 1000.0f);
            printf("  Kalman1:  %.1f mA (Q=0.1, R=5.0)\r\n", filtered_kalman1 * 1000.0f);
            printf("  Kalman2:  %.1f mA (Q=0.5, R=10.0)\r\n", filtered_kalman2 * 1000.0f);
            printf("  Noise Reduction: %.1f%%\r\n",
                   (1.0f - fabs(filtered_kalman1)/fabs(raw_current)) * 100.0f);
            printf("\r\n");
        }

        HAL_Delay(100);
    }

    printf("濾波器測試完成！\r\n");
}



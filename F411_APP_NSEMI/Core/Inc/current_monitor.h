#ifndef __CURRENT_MONITOR_H
#define __CURRENT_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "acs712.h"
#include "ssd1306.h"

/* 監控配置 */
#define OVERCURRENT_THRESHOLD   5.0f    // 過電流門檻 (A)
#define VOLTAGE_NOMINAL         5.0f //220.0f  // 標稱電壓 (V)
#define UPDATE_INTERVAL_MS      100     // 更新間隔 (ms)
#define FILTER_SIZE             10      // 濾波器大小

// 檢查這些值在 current_monitor.h 中的定義
#define FAN_5V_DETECTION_THRESHOLD  0.080f  // 50 mA
#define FAN_5V_STARTUP_THRESHOLD    0.100f  // 80 mA
#define FAN_5V_RUNNING_THRESHOLD    0.200f  // 150 mA
#define FAN_5V_NOISE_THRESHOLD      0.050f  // 20 mA

// 死區設定
#define CURRENT_DEADBAND           0.010f     // 40mA 死區

// 新增狀態枚舉
typedef enum {
    FAN_STOPPED = 0,
    FAN_STARTING,
    FAN_RUNNING,
    FAN_STOPPING
} Fan_Status_t;


/* 監控狀態 */
typedef enum {
    MONITOR_NORMAL = 0,
    MONITOR_OVERCURRENT,
    MONITOR_ERROR
} Monitor_Status_t;

/* 監控結構 */
typedef struct {
    ACS712_Handle_t *acs712;
    Current_Stats_t stats;
    Monitor_Status_t status;
    float current_now;          // 新增：當下電流值
    float filter_buffer[FILTER_SIZE];
    uint8_t filter_index;
    uint32_t last_update;
    float voltage;              // 系統電壓
    float power;               // 功率
    uint32_t energy_wh;        // 累積電能 (Wh)
    uint32_t energy_start_time; // 電能計算開始時間
} Current_Monitor_t;

/* 函數宣告 */
HAL_StatusTypeDef CurrentMonitor_Init(Current_Monitor_t *monitor, ACS712_Handle_t *acs712);
void CurrentMonitor_Update(Current_Monitor_t *monitor);
void CurrentMonitor_Display(Current_Monitor_t *monitor);
void CurrentMonitor_CheckOvercurrent(Current_Monitor_t *monitor);
float CurrentMonitor_MovingAverage(Current_Monitor_t *monitor, float new_value);
void CurrentMonitor_CalculatePower(Current_Monitor_t *monitor, float current);
void CurrentMonitor_ResetEnergy(Current_Monitor_t *monitor);
void CurrentMonitor_ResetStats(Current_Monitor_t *monitor);
void CurrentMonitor_ManualCalibration(Current_Monitor_t *monitor);
void CheckAutoReset(Current_Monitor_t *monitor);
void CurrentMonitor_ResetMovingAverage(Current_Monitor_t *monitor);
void CurrentMonitor_UpdateStats(Current_Monitor_t *monitor, float current);
void CurrentMonitor_TestFilters(Current_Monitor_t *monitor);
#ifdef __cplusplus
}
#endif

#endif /* __CURRENT_MONITOR_H */

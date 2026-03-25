/**
 * @file sysmon_driver.h
 * @brief System Monitor Driver - Performance & Health Monitoring
 * @details API để monitor system health:
 *          - CPU load (%)
 *          - Task timing stats
 *          - Memory usage (optional)
 *          - Uptime counter
 * @author STM32 System Monitor Team
 * @date 2026-03-22
 * 
 * ============================================================================
 * MỤC ĐÍCH: Cung cấp unified interface để monitor system performance
 * 
 * Metrics được track:
 *   - CPU utilization (%)
 *   - System uptime (ms)
 *   - Idle time percentage
 *   - Task execution time stats
 *   - Heap/Stack usage (optional)
 * 
 * Ví dụ sử dụng:
 *   SysMon_Init();                     // Khởi tạo monitoring
 *   SysMon_TaskStarted(task_id);       // Mark task started
 *   SysMon_TaskFinished(task_id);      // Mark task finished
 *   uint8_t cpu_load = SysMon_GetCPULoad();  // Đọc CPU %
 *   uint32_t uptime = SysMon_GetUptime();    // Đọc uptime (ms)
 * 
 * ============================================================================
 */

#ifndef SYSMON_DRIVER_H
#define SYSMON_DRIVER_H

#include <stdint.h>
#include "stm32f4xx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * SYSTEM MONITOR CONFIGURATION
 * ============================================================================ */

/** System monitor update rate (Hz) */
#define SYSMON_UPDATE_RATE     10    // Update every 100ms

/** Maximum number of tasks to track */
#define SYSMON_MAX_TASKS       5     // 5 RTOS tasks

/** CPU load calculation window (ms) */
#define SYSMON_WINDOW_MS       1000  // 1 second window

/* ============================================================================
 * SYSTEM MONITOR DATA STRUCTURES
 * ============================================================================ */

/**
 * @struct SysMon_TaskStats_t
 * @brief Per-task execution statistics
 */
typedef struct {
    uint32_t task_id;           // RTOS task ID or name
    uint32_t execution_count;   // Number of times executed
    uint32_t total_time_us;     // Total execution time (microseconds)
    uint32_t avg_time_us;       // Average execution time
    uint32_t max_time_us;       // Maximum execution time
    uint32_t min_time_us;       // Minimum execution time
} SysMon_TaskStats_t;

/**
 * @struct SysMon_SystemStats_t
 * @brief Overall system statistics
 */
typedef struct {
    uint32_t uptime_ms;         // System uptime (milliseconds)
    uint32_t total_cpu_time_us; // Total CPU time (microseconds)
    uint32_t total_idle_time_us;// Total idle time (microseconds)
    uint8_t  cpu_load_pct;      // CPU utilization (0-100%)
    uint8_t  cpu_idle_pct;      // CPU idle (0-100%)
    uint32_t context_switches;  // RTOS context switches count
} SysMon_SystemStats_t;

/* ============================================================================
 * KHAI BÁO HÀM TOÀN CỤC
 * ============================================================================ */

/**
 * @brief Khởi tạo System Monitor
 * 
 * @details Hàm này:
 *  1. Khởi tạo counters + timers
 *  2. Setup SysTick để tracking time
 *  3. Reset all task statistics
 *  4. Khởi tạo measurement window
 * 
 * @return void
 * @note Phải gọi từ SystemInit() hoặc main() trước hết
 */
void SysMon_Init(void);

/**
 * @brief Mark task execution started
 * 
 * @param[in] task_id  Task ID (0-4 cho 5 tasks)
 * 
 * @return void
 * 
 * @details Hàm này:
 *  1. Record start time (timestamp)
 *  2. Setup untuk track execution time
 * 
 * @note Gọi từ điểm đầu mỗi task
 * 
 * @example
 *  // Inside Motor Control Task
 *  SysMon_TaskStarted(0);
 *  // ... do motor control work
 *  SysMon_TaskFinished(0);
 */
void SysMon_TaskStarted(uint8_t task_id);

/**
 * @brief Mark task execution finished
 * 
 * @param[in] task_id  Task ID (0-4 cho 5 tasks)
 * 
 * @return void
 * 
 * @details Hàm này:
 *  1. Record end time
 *  2. Calculate execution time = (end - start)
 *  3. Update task statistics (min/max/avg)
 *  4. Increment execution counter
 *  5. Update total CPU time
 * 
 * @note Gọi từ điểm cuối mỗi task (hoặc yield/block point)
 */
void SysMon_TaskFinished(uint8_t task_id);

/**
 * @brief Get CPU utilization percentage
 * 
 * @return uint8_t  CPU load (0-100%)
 *         0% = idle, 100% = fully loaded
 * 
 * @details Tính theo công thức:
 *  CPU% = (total_cpu_time / total_elapsed_time) * 100
 * 
 * @example
 *  uint8_t load = SysMon_GetCPULoad();
 *  if (load > 80) {
 *      // System very busy - warn about timing
 *  }
 */
uint8_t SysMon_GetCPULoad(void);

/**
 * @brief Get system uptime
 * 
 * @return uint32_t  Uptime in milliseconds
 * 
 * @details Elapsed time từ lần init system
 * 
 * @example
 *  uint32_t uptime_ms = SysMon_GetUptime();
 *  uint32_t uptime_sec = uptime_ms / 1000;
 *  printf("Systems running for %d seconds\n", uptime_sec);
 */
uint32_t SysMon_GetUptime(void);

/**
 * @brief Get per-task statistics
 * 
 * @param[in] task_id  Task ID (0-4)
 * @param[out] pStats  Con trỏ tới SysMon_TaskStats_t
 * 
 * @return uint8_t  1 nếu thành công, 0 nếu task_id invalid
 * 
 * @details Trả về execution statistics cho một task cụ thể
 * 
 * @example
 *  SysMon_TaskStats_t stats;
 *  SysMon_GetTaskStats(0, &stats);
 *  printf("Motor Control Task avg: %d us\n", stats.avg_time_us);
 *  printf("Motor Control Task max: %d us\n", stats.max_time_us);
 */
uint8_t SysMon_GetTaskStats(uint8_t task_id, SysMon_TaskStats_t *pStats);

/**
 * @brief Get overall system statistics
 * 
 * @param[out] pStats  Con trỏ tới SysMon_SystemStats_t
 * 
 * @return void
 * 
 * @example
 *  SysMon_SystemStats_t sys_stats;
 *  SysMon_GetSystemStats(&sys_stats);
 *  printf("CPU Load: %d%%\n", sys_stats.cpu_load_pct);
 *  printf("Uptime: %d ms\n", sys_stats.uptime_ms);
 */
void SysMon_GetSystemStats(SysMon_SystemStats_t *pStats);

/**
 * @brief Get CPU idle percentage
 * 
 * @return uint8_t  Idle percentage (0-100%)
 *         = 100% - CPU_Load%
 * 
 * @example
 *  uint8_t idle = SysMon_GetIdleLoad();
 *  if (idle < 5) {
 *      // Very little idle time - system stressed
 *      printf("Warning: System at %d%% CPU\n", 100 - idle);
 *  }
 */
uint8_t SysMon_GetIdleLoad(void);

/**
 * @brief Get task execution count
 * 
 * @param[in] task_id  Task ID (0-4)
 * 
 * @return uint32_t  Number of times task executed
 * 
 * @details Useful for detecting if task is hung or stalled
 * 
 * @example
 *  uint32_t count1 = SysMon_GetTaskExecutionCount(0);
 *  osDelay(1000);
 *  uint32_t count2 = SysMon_GetTaskExecutionCount(0);
 *  if (count2 == count1) {
 *      // Task did not execute - might be hung
 *  }
 */
uint32_t SysMon_GetTaskExecutionCount(uint8_t task_id);

/**
 * @brief Reset all statistics
 * 
 * @return void
 * 
 * @details Clear counters, útful after diagnostics or restart
 */
void SysMon_ResetStats(void);

/**
 * @brief Record idle time (called from idle/system task)
 * 
 * @return void
 * 
 * @details Hàm này được gọi khi RTOS ở idle state
 * để track idle percentage và CPU load calculation
 * 
 * Note: Nên gọi từ FreeRTOS idle hook hoặc tương tự
 */
void SysMon_RecordIdleTime(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSMON_DRIVER_H */

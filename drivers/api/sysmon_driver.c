/**
 * @file sysmon_driver.c
 * @brief System Monitor Driver Implementation
 * @date 2026-03-22
 */

#include "sysmon_driver.h"
#include <stddef.h>

/* ============================================================================
 * PRIVATE VARIABLES
 * ============================================================================ */

/** Task statistics array */
static SysMon_TaskStats_t task_stats[SYSMON_MAX_TASKS];

/** System statistics */
static SysMon_SystemStats_t system_stats = {0};

/** Task start time (for current execution) */
/* static uint32_t task_start_time[SYSMON_MAX_TASKS]; */

/** Overall system start time */
/* static uint32_t system_start_time = 0; */

/** Idle time tracking */
/* static uint32_t idle_time_total = 0;
static uint32_t idle_time_window = 0; */

/* ============================================================================
 * PUBLIC FUNCTIONS - IMPLEMENTATION
 * ============================================================================ */

/**
 * @brief SysMon_Init - Khởi tạo System Monitor
 */
void SysMon_Init(void)
{
    /*
     * TODO: Initialize system monitoring
     * 
     * 1. Initialize task stats array:
     *    - memset(task_stats, 0, sizeof(task_stats))
     *    - Set task_id for each entry (0-4)
     * 
     * 2. Initialize task start time array:
     *    - memset(task_start_time, 0, sizeof(task_start_time))
     * 
     * 3. Initialize system stats:
     *    - memset(&system_stats, 0, sizeof(system_stats))
     *    - system_start_time = current_time_ms
     * 
     * 4. Setup SysTick counter for timing
     *    - Note: SysTick should already be running from SystemInit()
     */
}

/**
 * @brief SysMon_TaskStarted - Mark task start
 */
void SysMon_TaskStarted(uint8_t task_id)
{
    (void)task_id;  // Mark as used (TODO: implement)
    /*
     * TODO: Record task start time
     * 
     * 1. Validate task_id (0 - SYSMON_MAX_TASKS-1)
     *    if (task_id >= SYSMON_MAX_TASKS) return
     * 
     * 2. Get current timestamp (in microseconds or ticks)
     *    task_start_time[task_id] = SysTick_GetMicroseconds()
     *    or similar timing function
     * 
     * Note: Use microsecond resolution for accurate CPU timing
     */
}

/**
 * @brief SysMon_TaskFinished - Mark task finished
 */
void SysMon_TaskFinished(uint8_t task_id)
{
    (void)task_id;  // Mark as used (TODO: implement)
    /*
     * TODO: Calculate and record task execution time
     * 
     * 1. Validate task_id
     *    if (task_id >= SYSMON_MAX_TASKS) return
     * 
     * 2. Calculate execution time:
     *    end_time = SysTick_GetMicroseconds()
     *    exec_time = end_time - task_start_time[task_id]
     * 
     * 3. Update task statistics:
     *    task_stats[task_id].execution_count++
     *    task_stats[task_id].total_time_us += exec_time
     * 
     * 4. Update min/max:
     *    if (exec_time > task_stats[task_id].max_time_us)
     *        task_stats[task_id].max_time_us = exec_time
     *    if (exec_time < task_stats[task_id].min_time_us)
     *        task_stats[task_id].min_time_us = exec_time
     * 
     * 5. Calculate average:
     *    task_stats[task_id].avg_time_us = 
     *        task_stats[task_id].total_time_us / task_stats[task_id].execution_count
     * 
     * 6. Update system stats:
     *    system_stats.total_cpu_time_us += exec_time
     */
}

/**
 * @brief SysMon_GetCPULoad - Get CPU utilization %
 */
uint8_t SysMon_GetCPULoad(void)
{
    /*
     * TODO: Calculate CPU load percentage
     * 
     * Formula:
     *  elapsed_time = current_time - system_start_time
     *  cpu_load_pct = (system_stats.total_cpu_time_us / elapsed_time) * 100
     * 
     * Example:
     *  If total CPU time = 800ms over 1000ms window
     *  CPU load = (800/1000)*100 = 80%
     * 
     * Clamp to 0-100%
     */
    
    return system_stats.cpu_load_pct;
}

/**
 * @brief SysMon_GetUptime - Get system uptime
 */
uint32_t SysMon_GetUptime(void)
{
    /*
     * TODO: Calculate system uptime
     * 
     * uptime_ms = (current_time_us - system_start_time) / 1000
     */
    
    return system_stats.uptime_ms;
}

/**
 * @brief SysMon_GetTaskStats - Get per-task stats
 */
uint8_t SysMon_GetTaskStats(uint8_t task_id, SysMon_TaskStats_t *pStats)
{
    /*
     * TODO: Return task statistics
     * 
     * 1. Validate inputs
     *    if (task_id >= SYSMON_MAX_TASKS || pStats == NULL) return 0
     * 
     * 2. Copy data
     *    *pStats = task_stats[task_id]
     * 
     * 3. Return 1 (success)
     */
    
    if (task_id >= SYSMON_MAX_TASKS || pStats == NULL) return 0;
    
    *pStats = task_stats[task_id];
    return 1;
}

/**
 * @brief SysMon_GetSystemStats - Get system stats
 */
void SysMon_GetSystemStats(SysMon_SystemStats_t *pStats)
{
    /*
     * TODO: Return system statistics
     * 
     * Copy current system_stats to pStats
     */
    
    if (pStats == NULL) return;
    
    *pStats = system_stats;
}

/**
 * @brief SysMon_GetIdleLoad - Get idle percentage
 */
uint8_t SysMon_GetIdleLoad(void)
{
    /*
     * TODO: Calculate idle percentage
     * 
     * idle_pct = 100 - cpu_load_pct
     */
    
    return system_stats.cpu_idle_pct;
}

/**
 * @brief SysMon_GetTaskExecutionCount - Get task execution count
 */
uint32_t SysMon_GetTaskExecutionCount(uint8_t task_id)
{
    /*
     * TODO: Return execution count for task
     * 
     * if (task_id >= SYSMON_MAX_TASKS) return 0
     * return task_stats[task_id].execution_count
     */
    
    if (task_id >= SYSMON_MAX_TASKS) return 0;
    
    return task_stats[task_id].execution_count;
}

/**
 * @brief SysMon_ResetStats - Reset statistics
 */
void SysMon_ResetStats(void)
{
    /*
     * TODO: Reset all statistics (same as Init)
     */
}

/**
 * @brief SysMon_RecordIdleTime - Record idle time
 */
void SysMon_RecordIdleTime(void)
{
    /*
     * TODO: Track idle time for CPU load calculation
     * 
     * idle_time_total += time_since_last_call
     * This is called from RTOS idle hook
     */
}

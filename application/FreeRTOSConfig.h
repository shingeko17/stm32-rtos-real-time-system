/**
 * @file FreeRTOSConfig.h
 * @brief FreeRTOS Configuration for STM32F407
 * @date 2026-03-22
 */

#ifndef FREERTOSCONFIG_H
#define FREERTOSCONFIG_H

#include "stm32f4xx.h"

/* ============================================================================
 * FREERTOS CORE CONFIGURATION
 * ============================================================================ */

#define configUSE_PREEMPTION                     1
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configTICK_RATE_HZ                       1000
#define configMAX_PRIORITIES                     5
#define configMINIMAL_STACK_SIZE                 128
#define configTOTAL_HEAP_SIZE                    8192
#define configMAX_TASK_NAME_LEN                  16
#define configUSE_16_BIT_TICKS                   0
#define configIDLE_SHOULD_YIELD                  1

/* ============================================================================
 * MEMORY ALLOCATION
 * ============================================================================ */

#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configSUPPORT_STATIC_ALLOCATION          0

/* ============================================================================
 * HOOK FUNCTIONS
 * ============================================================================ */

#define configUSE_MALLOC_FAILED_HOOK             1
#define configCHECK_FOR_STACK_OVERFLOW           0

/* ============================================================================
 * RUN TIME STATS
 * ============================================================================ */

#define configGENERATE_RUN_TIME_STATS            0
#define configUSE_TRACE_FACILITY                 0

/* ============================================================================
 * CO-ROUTINES
 * ============================================================================ */

#define configUSE_CO_ROUTINES                    0
#define configMAX_CO_ROUTINE_PRIORITIES          2

/* ============================================================================
 * QUEUE MANAGEMENT
 * ============================================================================ */

#define configUSE_QUEUE_SETS                     0

/* ============================================================================
 * TASK NOTIFICATIONS
 * ============================================================================ */

#define configUSE_TASK_NOTIFICATIONS             1

/* ============================================================================
 * MUTEX AND SEMAPHORES
 * ============================================================================ */

#define configUSE_MUTEXES                        1
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_COUNTING_SEMAPHORES            1

/* ============================================================================
 * DEADLOCK DETECTION
 * ============================================================================ */

#define configUSE_ALTERNATIVE_API                0

/* ============================================================================
 * ISR NESTING
 * ============================================================================ */

#define configKERNEL_INTERRUPT_PRIORITY          191
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     176

/* ============================================================================
 * PORT SPECIFIC CONFIGURATION
 * ============================================================================ */

#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1

/* ============================================================================
 * ASSERTION
 * ============================================================================ */

#define configASSERT(x) if ((x) == 0) { vAssertCalled(__FILE__, __LINE__); }

void vAssertCalled(const char *pcFile, unsigned long ulLine);

/* ============================================================================
 * FREERTOS VERSION REFERENCES - TIME HELPERS
 * ============================================================================ */

#define pdMS_TO_TICKS(xTimeInMs) (((TickType_t)(xTimeInMs) * configTICK_RATE_HZ) / 1000)

#endif /* FREERTOSCONFIG_H */

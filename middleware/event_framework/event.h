/**
 * @file event.h
 * @brief Event-Driven Framework - Core Event Definitions
 * @details Triển khai mô hình Active Objects từ Quantum Leaps
 *          - Event signals và parameters
 *          - Event queue cho thread-safe communication
 *          - Run-to-Completion (RTC) event processing
 * @date 2026-04-01
 *
 * ============================================================================
 * KIẾN TRÚC EVENT-DRIVEN (từ bài Beyond the RTOS):
 * ============================================================================
 * 
 * ✓ KHÔNG BLOCKING: Các active objects gửi events asynchronously
 * ✓ KHÔNG CHIA SẺ DỮ LIỆU: Mỗi AO có dữ liệu private + encapsulation
 * ✓ RUN-TO-COMPLETION: Xử lý 1 event xong mới xử lý event tiếp theo
 * 
 * Lợi ích so với bare-metal polling:
 * - Responsive: không bị block khi chờ event
 * - Scalable: dễ thêm events mới
 * - Testable: có thể mock events
 * - Safe: không cần mutex/semaphore
 */

#ifndef EVENT_FRAMEWORK_H
#define EVENT_FRAMEWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * EVENT SIGNAL DEFINITIONS
 * ============================================================================
 * 
 * Mỗi event có một signal (ID) để xác định loại event
 * Event cũng có thể kèm theo parameters (dữ liệu)
 */

typedef enum {
    /* Reserved signals */
    ENTRY_SIG = 0,          /* Signal khi vào state        */
    EXIT_SIG,               /* Signal khi ra khỏi state    */
    INIT_SIG,               /* Signal khởi tạo             */

    /* Motor control events */
    MOTOR_START_SIG,        /* Yêu cầu khởi động motor     */
    MOTOR_STOP_SIG,         /* Yêu cầu dừng motor          */
    MOTOR_SPEED_UPDATE_SIG, /* Cập nhật tốc độ             */
    MOTOR_OVERCURRENT_SIG,  /* Sự cố quá dòng              */
    MOTOR_OVERHEAT_SIG,     /* Sự cố quá nhiệt             */

    /* Sensor events */
    SENSOR_READ_SIG,        /* Yêu cầu đọc cảm biến        */
    SENSOR_UPDATE_SIG,      /* Cập nhật giá trị cảm biến   */

    /* Timer/Timeout events */
    TICK_SIG,               /* Ticks định kỳ (heartbeat)   */
    TIMEOUT_SIG,            /* Timeout xảy ra              */

    /* Communication events */
    UART_RX_SIG,            /* Nhận dữ liệu UART           */
    UART_TX_SIG,            /* Gửi dữ liệu UART            */

    /* System events */
    SYSTEM_ERROR_SIG,       /* Lỗi hệ thống                */
    USER_DEFINED_SIG = 100  /* Bắt đầu định nghĩa tùy chỉnh */
} EventSignal;

/* ============================================================================
 * EVENT STRUCTURE
 * ============================================================================
 */

/**
 * @typedef Event_t
 * @brief Base event structure
 * 
 * Mỗi event chứa:
 * - signal: loại event
 * - data: parameter của event (union để lưu nhiều kiểu)
 */
typedef struct {
    uint16_t sig;           /* Event signal ID */
    union {
        uint32_t u32;       /* 32-bit unsigned */
        int32_t  i32;       /* 32-bit signed   */
        uint16_t u16[2];    /* 2x 16-bit       */
        uint8_t  u8[4];     /* 4x 8-bit        */
        float    f;         /* Float value     */
        void    *ptr;       /* Pointer         */
    } data;
} Event_t;

/* ============================================================================
 * EVENT QUEUE - Message Pump Storage
 * ============================================================================
 *
 * Run-to-Completion Semantics:
 * - Thread chỉ block khi queue trống
 * - Xử lý event xong (RTC) mới lấy event tiếp theo
 * - Không block trong event handler
 */

#define EVENT_QUEUE_SIZE 16   /* Số lượng events tối đa trong queue */

typedef struct {
    Event_t   events[EVENT_QUEUE_SIZE];
    uint16_t  head;           /* Chỉ số đầu queue (lấy từ đây) */
    uint16_t  tail;           /* Chỉ số cuối queue (thêm vào đây) */
    uint16_t  count;          /* Số events hiện tại trong queue */
} EventQueue_t;

/* ============================================================================
 * EVENT QUEUE API
 * ============================================================================
 */

/**
 * @brief Khởi tạo event queue
 * @param[out] q Pointer đến queue cần khởi tạo
 */
void EventQueue_Init(EventQueue_t *q);

/**
 * @brief Đưa event vào queue (asynchronous post)
 * @param[in] q Pointer đến queue
 * @param[in] e Pointer đến event cần thêm
 * @return true nếu thành công, false nếu queue full
 */
bool EventQueue_Post(EventQueue_t *q, const Event_t *e);

/**
 * @brief Lấy event từ queue (blocking)
 * @param[in] q Pointer đến queue
 * @param[out] e Pointer để lưu event lấy ra
 * @return true nếu có event, false nếu queue trống
 */
bool EventQueue_Pend(EventQueue_t *q, Event_t *e);

/**
 * @brief Kiểm tra queue có trống không
 * @param[in] q Pointer đến queue
 * @return true nếu trống
 */
bool EventQueue_IsEmpty(const EventQueue_t *q);

/**
 * @brief Lấy số lượng events hiện tại
 * @param[in] q Pointer đến queue
 * @return Số events
 */
uint16_t EventQueue_Count(const EventQueue_t *q);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Tạo event đơn giản (chỉ có signal)
 * @param[in] sig Event signal
 * @return Event mới tạo
 */
Event_t Event_Create(EventSignal sig);

/**
 * @brief Tạo event với data u32
 * @param[in] sig Event signal
 * @param[in] val Giá trị u32
 * @return Event mới tạo
 */
Event_t Event_CreateWithU32(EventSignal sig, uint32_t val);

/**
 * @brief Tạo event với data i32
 * @param[in] sig Event signal
 * @param[in] val Giá trị i32
 * @return Event mới tạo
 */
Event_t Event_CreateWithI32(EventSignal sig, int32_t val);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_FRAMEWORK_H */

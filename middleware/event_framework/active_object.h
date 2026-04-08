/**
 * @file active_object.h
 * @brief Active Object Base Class
 * @details Active Objects (Actors) - Event-driven, encapsulated soft entities
 *          Mỗi AO:
 *          - Chạy trong thread riêng (hoặc chia sẻ một scheduler tập trung)
 *          - Có private data (encapsulation)
 *          - Có event queue riêng (message pump)
 *          - Xử lý events asynchronously (RTC - Run-to-Completion)
 *          - State machine cho behavior control
 * @date 2026-04-01
 */

#ifndef ACTIVE_OBJECT_H
#define ACTIVE_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "event.h"

/* ============================================================================
 * ACTIVE OBJECT STATE HANDLER
 * ============================================================================
 */

/* Forward declaration */
typedef struct ActiveObject ActiveObject_t;

/**
 * @typedef StateHandler_t
 * @brief Function pointer cho state handler
 * 
 * Mỗi state là một hàm handler:
 * - Nhận event hiện tại
 * - Quyết định transition hoặc action
 * - Return state mới (có thể là state hiện tại để stay in state)
 * 
 * @param[in] me Pointer đến AO
 * @param[in] e  Pointer đến event cần xử lý
 * @return Pointer đến state handler tiếp theo (NULL = stay)
 */
typedef StateHandler_t (*StateHandler_t)(ActiveObject_t *me, const Event_t *e);

/* ============================================================================
 * ACTIVE OBJECT STRUCTURE
 * ============================================================================
 */

/**
 * @struct ActiveObject
 * @brief Base Active Object
 * 
 * Mỗi AO có:
 * - Priority (nếu dùng scheduler)
 * - State (state machine)
 * - Event queue (message pump)
 * - User-defined data (private)
 */
typedef struct ActiveObject {
    uint8_t           priority;     /* Priority level (0 = idleest) */
    StateHandler_t    state;        /* Current state handler */
    EventQueue_t      queue;        /* Event queue (message pump) */
    void             *priv;         /* Private data pointer */
} ActiveObject_t;

/* ============================================================================
 * ACTIVE OBJECT API
 * ============================================================================
 */

/**
 * @brief Khởi tạo Active Object
 * @param[out] me       Pointer đến AO
 * @param[in]  priority Priority level
 * @param[in]  state    Initial state handler
 * @param[in]  priv     Private data pointer (có thể NULL)
 */
void ActiveObject_Init(
    ActiveObject_t  *me,
    uint8_t          priority,
    StateHandler_t   state,
    void            *priv
);

/**
 * @brief Gửi event tới Active Object (asynchronous post)
 * 
 * Đặc điểm:
 * - Non-blocking (không chờ event được xử lý)
 * - Thread-safe (có thể gọi từ ISR hoặc AO khác)
 * - Return ngay lập tức (event vào queue)
 * 
 * @param[in] me Pointer đến AO
 * @param[in] e  Pointer đến event
 * @return true nếu post thành công, false nếu queue full
 */
bool ActiveObject_Post(ActiveObject_t *me, const Event_t *e);

/**
 * @brief Lấy và xử lý một event từ queue (RTC step)
 * 
 * Đặc điểm RTC:
 * - Xử lý 1 event xong mới xử lý event tiếp theo
 * - Có thể trigger state transition
 * - Có thể post events mới (queued, không xử lý ngay)
 * 
 * @param[in] me Pointer đến AO
 * @return true nếu có event được xử lý, false nếu queue trống
 */
bool ActiveObject_Dispatch(ActiveObject_t *me);

/**
 * @brief Quá trình bắt tay khởi tạo (initial transition)
 * 
 * Gửi ENTRY_SIG tới initial state để setup
 * 
 * @param[in] me Pointer đến AO
 */
void ActiveObject_Start(ActiveObject_t *me);

/**
 * @brief Lấy state hiện tại
 * @param[in] me Pointer đến AO
 * @return State handler hiện tại
 */
StateHandler_t ActiveObject_GetState(const ActiveObject_t *me);

/**
 * @brief Set state mới (transition)
 * @param[in] me    Pointer đến AO
 * @param[in] state State handler mới
 */
void ActiveObject_SetState(ActiveObject_t *me, StateHandler_t state);

/**
 * @brief Kiểm tra queue có events không
 * @param[in] me Pointer đến AO
 * @return true nếu queue có events
 */
bool ActiveObject_HasEvents(const ActiveObject_t *me);

/**
 * @brief Lấy số lượng events hiện tại trong queue
 * @param[in] me Pointer đến AO
 * @return Số events
 */
uint16_t ActiveObject_EventCount(const ActiveObject_t *me);

/* ============================================================================
 * SCHEDULER - Người quản lý tất cả Active Objects
 * ============================================================================
 * 
 * Cooperative scheduler (QV kernel style từ bài giảng):
 * - Chọn AO có priority cao nhất và có events
 * - Xử lý 1 RTC step của AO đó
 * - Quay lại chọn AO tiếp theo
 * - Khi tất cả queue trống → call idle callback (sleep CPU)
 */

#define MAX_ACTIVE_OBJECTS 8

typedef struct {
    ActiveObject_t *aos[MAX_ACTIVE_OBJECTS];
    uint8_t         count;
    void          (*idle_callback)(void);  /* Gọi khi tất cả queue trống */
} Scheduler_t;

/**
 * @brief Khởi tạo scheduler
 * @param[out] sched      Pointer đến scheduler
 * @param[in]  idle_cb    Callback khi idle (có thể NULL)
 */
void Scheduler_Init(Scheduler_t *sched, void (*idle_cb)(void));

/**
 * @brief Đăng ký Active Object vào scheduler
 * @param[in] sched Pointer đến scheduler
 * @param[in] ao    Pointer đến AO cần đăng ký
 * @return true nếu đăng ký thành công, false nếu scheduler đầy
 */
bool Scheduler_Register(Scheduler_t *sched, ActiveObject_t *ao);

/**
 * @brief Chạy scheduler (vòng lặp chính)
 * 
 * Điều hành tất cả AOs:
 * 1. Tìm AO có priority cao nhất với events
 * 2. Dispatch 1 RTC step
 * 3. Nếu tất cả queue trống → gọi idle_callback
 * 4. Quay lại bước 1
 * 
 * @param[in] sched Pointer đến scheduler
 * 
 * @note Hàm này là vòng lặp vô tận, không return
 */
void Scheduler_Run(Scheduler_t *sched);

/**
 * @brief One step của scheduler (tiện cho testing)
 * @param[in] sched Pointer đến scheduler
 * @return true nếu có event được xử lý
 */
bool Scheduler_Step(Scheduler_t *sched);

#ifdef __cplusplus
}
#endif

#endif /* ACTIVE_OBJECT_H */

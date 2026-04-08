/**
 * @file motor_controller_ao.h
 * @brief Motor Controller Active Object
 * @details Điều khiển motor theo kiến trúc event-driven
 *          
 * State Machine:
 *   [Ready] → (MOTOR_START) → [Running] → (MOTOR_STOP) → [Ready]
 *      ↓                           ↓
 *   (Overcurrent/Overheat) → [Error]
 * 
 * @date 2026-04-01
 */

#ifndef MOTOR_CONTROLLER_AO_H
#define MOTOR_CONTROLLER_AO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "active_object.h"

/* ============================================================================
 * MOTOR CONTROLLER STATES
 * ============================================================================
 */

typedef enum {
    MC_STATE_READY = 0,    /* Chờ lệnh khởi động */
    MC_STATE_RUNNING,      /* Motor đang quay */
    MC_STATE_ERROR,        /* Lỗi (quá dòng, quá nhiệt) */
} MotorControllerState;

/* ============================================================================
 * MOTOR CONTROLLER PRIVATE DATA
 * ============================================================================
 */

typedef struct {
    uint8_t          channel;        /* Channel motor: 0=A, 1=B */
    int16_t          speed_cmd;      /* Lệnh tốc độ (-100..+100) */
    uint16_t         measured_rpm;   /* RPM đo được từ encoder */
    uint16_t         current_ma;     /* Dòng điện hiện tại (mA) */
    uint16_t         temp_c;         /* Nhiệt độ (°C) */
    uint16_t         max_current_ma; /* Giới hạn dòng điện */
    uint16_t         max_temp_c;     /* Giới hạn nhiệt độ */
    MotorControllerState state;      /* State machine */
} MotorControllerData_t;

/* ============================================================================
 * MOTOR CONTROLLER ACTIVE OBJECT
 * ============================================================================
 */

/**
 * @brief Tạo và khởi tạo Motor Controller AO
 * @param[in] channel Channel motor (0=A, 1=B)
 * @param[in] priority Priority level
 * @return Pointer đến ActiveObject_t (chứa MotorControllerData_t)
 * 
 * @note Bộ nhớ được cấp phát động, cần gọi MotorController_Destroy() để giải phóng
 */
ActiveObject_t* MotorController_Create(uint8_t channel, uint8_t priority);

/**
 * @brief Xóa Motor Controller AO
 * @param[in] ao Pointer đến AO
 */
void MotorController_Destroy(ActiveObject_t *ao);

/**
 * @brief Gửi lệnh khởi động motor
 * @param[in] ao Pointer đến AO
 * @param[in] speed Tốc độ (-100..+100)
 * @return true nếu post thành công
 */
bool MotorController_Start(ActiveObject_t *ao, int16_t speed);

/**
 * @brief Gửi lệnh dừng motor
 * @param[in] ao Pointer đến AO
 * @return true nếu post thành công
 */
bool MotorController_Stop(ActiveObject_t *ao);

/**
 * @brief Cập nhật tốc độ (khi motor đang chạy)
 * @param[in] ao Pointer đến AO
 * @param[in] speed Tốc độ mới (-100..+100)
 * @return true nếu post thành công
 */
bool MotorController_UpdateSpeed(ActiveObject_t *ao, int16_t speed);

/**
 * @brief Gửi sự kiện sensors (dòng điện và nhiệt độ)
 * @param[in] ao Pointer đến AO
 * @param[in] current_ma Dòng điện (mA)
 * @param[in] temp_c Nhiệt độ (°C)
 * @return true nếu post thành công
 */
bool MotorController_UpdateSensors(ActiveObject_t *ao, uint16_t current_ma, uint16_t temp_c);

/**
 * @brief Lấy trạng thái motor hiện tại
 * @param[in] ao Pointer đến AO
 * @return State hiện tại
 */
MotorControllerState MotorController_GetState(const ActiveObject_t *ao);

/**
 * @brief Lấy dữ liệu private (debug/monitoring)
 * @param[in] ao Pointer đến AO
 * @return Pointer đến MotorControllerData_t
 */
MotorControllerData_t* MotorController_GetData(const ActiveObject_t *ao);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_CONTROLLER_AO_H */

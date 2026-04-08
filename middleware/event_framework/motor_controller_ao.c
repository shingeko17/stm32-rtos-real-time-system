/**
 * @file motor_controller_ao.c
 * @brief Motor Controller Active Object Implementation
 * @details State machine handlers cho motor control
 * @date 2026-04-01
 */

#include "motor_controller_ao.h"
#include "motor_driver.h"
#include "stdlib.h"

/* ============================================================================
 * STATE HANDLERS (Forward declarations)
 * ============================================================================
 */

static StateHandler_t State_Ready(ActiveObject_t *me, const Event_t *e);
static StateHandler_t State_Running(ActiveObject_t *me, const Event_t *e);
static StateHandler_t State_Error(ActiveObject_t *me, const Event_t *e);

/* ============================================================================
 * MOTOR CONTROLLER CREATION / DESTRUCTION
 * ============================================================================
 */

ActiveObject_t* MotorController_Create(uint8_t channel, uint8_t priority)
{
    /* Cấp phát bộ nhớ cho AO */
    ActiveObject_t *ao = (ActiveObject_t *)malloc(sizeof(ActiveObject_t));
    if (ao == NULL) return NULL;
    
    /* Cấp phát bộ nhớ cho private data */
    MotorControllerData_t *data = (MotorControllerData_t *)malloc(sizeof(MotorControllerData_t));
    if (data == NULL) {
        free(ao);
        return NULL;
    }
    
    /* Khởi tạo private data */
    data->channel = channel;
    data->speed_cmd = 0;
    data->measured_rpm = 0;
    data->current_ma = 0;
    data->temp_c = 0;
    data->max_current_ma = 3500;  /* Default safety limit */
    data->max_temp_c = 85;         /* Default safety limit */
    data->state = MC_STATE_READY;
    
    /* Khởi tạo Active Object */
    ActiveObject_Init(ao, priority, State_Ready, (void *)data);
    
    return ao;
}

void MotorController_Destroy(ActiveObject_t *ao)
{
    if (ao == NULL) return;
    
    if (ao->priv != NULL) {
        free(ao->priv);
    }
    free(ao);
}

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================
 */

static inline MotorControllerData_t* GetData(ActiveObject_t *me)
{
    return (MotorControllerData_t *)me->priv;
}

static inline void RunMotor(MotorControllerData_t *data)
{
    uint8_t channel = data->channel;
    int16_t speed = data->speed_cmd;
    
    if (channel == 0) {
        Motor_A_Run(speed);
    } else {
        Motor_B_Run(speed);
    }
}

static inline void StopMotor(MotorControllerData_t *data)
{
    uint8_t channel = data->channel;
    
    if (channel == 0) {
        Motor_A_Run(0);
    } else {
        Motor_B_Run(0);
    }
}

/* ============================================================================
 * STATE HANDLERS
 * ============================================================================
 */

/**
 * State: Ready
 * - Chờ MOTOR_START event
 * - Xử lý ENTRY_SIG: dừng motor
 */
static StateHandler_t State_Ready(ActiveObject_t *me, const Event_t *e)
{
    if (me == NULL || e == NULL) return NULL;
    
    MotorControllerData_t *data = GetData(me);
    
    switch (e->sig) {
        case ENTRY_SIG:
            /* Entry action: dừng motor */
            StopMotor(data);
            data->state = MC_STATE_READY;
            break;
            
        case MOTOR_START_SIG:
            /* Lấy tốc độ từ event data */
            data->speed_cmd = e->data.i32;
            RunMotor(data);
            
            /* Transition → Running */
            return State_Running;
            
        case EXIT_SIG:
            /* Không cần action gì */
            break;
            
        default:
            /* Ignore other events */
            break;
    }
    
    return NULL;  /* Stay in Ready state */
}

/**
 * State: Running
 * - Xử lý MOTOR_SPEED_UPDATE_SIG: cập nhật tốc độ
 * - Xử lý MOTOR_OVERCURRENT_SIG: → Error
 * - Xử lý MOTOR_OVERHEAT_SIG: → Error
 * - Xử lý MOTOR_STOP_SIG: → Ready
 */
static StateHandler_t State_Running(ActiveObject_t *me, const Event_t *e)
{
    if (me == NULL || e == NULL) return NULL;
    
    MotorControllerData_t *data = GetData(me);
    
    switch (e->sig) {
        case ENTRY_SIG:
            /* Entry action: motor đã đang quay */
            data->state = MC_STATE_RUNNING;
            break;
            
        case MOTOR_SPEED_UPDATE_SIG:
            /* Cập nhật tốc độ */
            data->speed_cmd = e->data.i32;
            RunMotor(data);
            break;
            
        case MOTOR_OVERCURRENT_SIG:
        case MOTOR_OVERHEAT_SIG:
            /* Error event → transition to Error state */
            return State_Error;
            
        case SENSOR_UPDATE_SIG:
            /* Cập nhật sensors (multi-part data) */
            data->current_ma = e->data.u16[0];
            data->temp_c = e->data.u16[1];
            
            /* Kiểm tra safety limits */
            if (data->current_ma > data->max_current_ma ||
                data->temp_c > data->max_temp_c) {
                /* Safety violation → Error state */
                return State_Error;
            }
            break;
            
        case MOTOR_STOP_SIG:
            /* Stop request → Ready */
            return State_Ready;
            
        case EXIT_SIG:
            /* Exit action: dừng motor khi rời state */
            StopMotor(data);
            break;
            
        default:
            break;
    }
    
    return NULL;  /* Stay in Running state */
}

/**
 * State: Error
 * - Xử lý ENTRY_SIG: dừng motor, log error
 * - Xử lý MOTOR_STOP_SIG: → Ready (clear error)
 */
static StateHandler_t State_Error(ActiveObject_t *me, const Event_t *e)
{
    if (me == NULL || e == NULL) return NULL;
    
    MotorControllerData_t *data = GetData(me);
    
    switch (e->sig) {
        case ENTRY_SIG:
            /* Entry action: emergency stop */
            StopMotor(data);
            data->state = MC_STATE_ERROR;
            /* TODO: Log error condition */
            break;
            
        case MOTOR_STOP_SIG:
            /* Clear error by stopping and returning to Ready */
            return State_Ready;
            
        case EXIT_SIG:
            /* Exit action */
            break;
            
        default:
            break;
    }
    
    return NULL;  /* Stay in Error state */
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================
 */

bool MotorController_Start(ActiveObject_t *ao, int16_t speed)
{
    if (ao == NULL) return false;
    
    Event_t e = Event_CreateWithI32(MOTOR_START_SIG, (int32_t)speed);
    return ActiveObject_Post(ao, &e);
}

bool MotorController_Stop(ActiveObject_t *ao)
{
    if (ao == NULL) return false;
    
    Event_t e = Event_Create(MOTOR_STOP_SIG);
    return ActiveObject_Post(ao, &e);
}

bool MotorController_UpdateSpeed(ActiveObject_t *ao, int16_t speed)
{
    if (ao == NULL) return false;
    
    Event_t e = Event_CreateWithI32(MOTOR_SPEED_UPDATE_SIG, (int32_t)speed);
    return ActiveObject_Post(ao, &e);
}

bool MotorController_UpdateSensors(ActiveObject_t *ao, uint16_t current_ma, uint16_t temp_c)
{
    if (ao == NULL) return false;
    
    Event_t e;
    e.sig = SENSOR_UPDATE_SIG;
    e.data.u16[0] = current_ma;
    e.data.u16[1] = temp_c;
    
    return ActiveObject_Post(ao, &e);
}

MotorControllerState MotorController_GetState(const ActiveObject_t *ao)
{
    if (ao == NULL) return MC_STATE_READY;
    
    MotorControllerData_t *data = (MotorControllerData_t *)ao->priv;
    if (data == NULL) return MC_STATE_READY;
    
    return data->state;
}

MotorControllerData_t* MotorController_GetData(const ActiveObject_t *ao)
{
    if (ao == NULL) return NULL;
    return (MotorControllerData_t *)ao->priv;
}

/**
 * @file EVENT_DRIVEN_GUIDE.md
 * @brief Event-Driven Architecture Integration Guide
 * @date 2026-04-01
 */

# Event-Driven Architecture Integration Guide

## 📋 Tổng Quan

Bạn đã học từ bài giảng **"Beyond the RTOS"** rằng kiến trúc event-driven với **Active Objects** (Actors) tốt hơn bare-metal polling và RTOS truyền thống.

### So Sánh 3 Kiến Trúc:

```
┌─────────────────────────────────────────────────────────────────┐
│  1. BARE-METAL POLLING (CỰ)                                     │
├─────────────────────────────────────────────────────────────────┤
while(1) {
    if (time_elapsed) {
        speed = ADC_Read();
        if (speed > MAX) Motor_Stop();
        else Motor_Run(speed);
    }
}

✓ Đơn giản
✗ Unresponsive (block khi chờ)
✗ Khó mở rộng
✗ "Spaghetti code" khi phức tạp
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  2. RTOS (Threads + Semaphores) (RẤT XẤU)                       │
├─────────────────────────────────────────────────────────────────┤
Thread_Start(motor_thread);

void motor_thread() {
    while(1) {
        OSSem_Wait(&speed_sem);      // BLOCKING!
        speed = ADC_Read();
        Motor_Run(speed);
    }
}

✓ Separé concerns
✗ BLOCKING → priority inversion, deadlock, starvation
✗ Cần stack cho mỗi thread
✗ Cần mutex/semaphore → phức tạp
✗ Hard-to-debug concurrency bugs
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  3. EVENT-DRIVEN (Active Objects) (TỐI ƯU) ✓                    │
├─────────────────────────────────────────────────────────────────┤
/* Post events asynchronously - NO BLOCKING */
MotorController_UpdateSpeed(motor_ao, speed);

/* State machine xử lý events */
State_Running() {
    switch(event) {
        case SPEED_UPDATE: RunMotor(); break;
        case OVERCURRENT: goto Error; break;
    }
}

✓ Responsive (không block)
✓ Scalable (thêm events dễ)
✓ Safe (RTC - Run-To-Completion)
✓ Efficient (ít stack, ít overhead)
✓ Testable (mock events)
└─────────────────────────────────────────────────────────────────┘
```

---

## 🏗️ Architecture Layers

```
┌─────────────────────────────────────┐
│  Application Layer                  │
│  ├─ Motor Controller AO             │ ← Bạn phát triển
│  ├─ Sensor Sampler AO               │
│  └─ Communication Manager AO        │
├─────────────────────────────────────┤
│  Event-Driven Framework             │
│  ├─ Scheduler (Cooperative QV)      │ ← Framework cung cấp
│  ├─ Active Object Base              │
│  ├─ Event Queue                     │
│  └─ State Machine Processor         │
├─────────────────────────────────────┤
│  Hardware Abstraction               │
│  ├─ Drivers (Motor, ADC, UART)      │
│  ├─ BSP (Board Support Package)     │
│  └─ System Init                     │
├─────────────────────────────────────┤
│  Target Hardware                    │
│  └─ STM32F4                         │
└─────────────────────────────────────┘
```

---

## 🔑 Key Concepts

### 1. **Event (Sự Kiện)**
```c
typedef struct {
    uint16_t sig;       /* Event signal ID (loại event) */
    union {
        uint32_t u32;   /* 32-bit dữ liệu */
        int32_t  i32;   /* 32-bit signed */
        float    f;     /* Float */
        void    *ptr;   /* Pointer */
    } data;
} Event_t;
```

**Ví dụ:**
```c
Event_t speed_event = Event_CreateWithI32(MOTOR_SPEED_UPDATE_SIG, 75);
// Nghĩa: "Cập nhật tốc độ motor thành 75%"
```

### 2. **Active Object (Tác nhân Chủ động)**
```c
typedef struct {
    uint8_t       priority;    /* Độ ưu tiên (0 = cao nhất) */
    StateHandler_t state;      /* State handler hiện tại */
    EventQueue_t  queue;       /* Message pump - buffer events */
    void         *priv;        /* Private data */
} ActiveObject_t;
```

**Tính chất:**
- 🔒 **Encapsulated**: Dữ liệu private, chỉ communicate qua events
- 📮 **Message Queue**: Nhận events asynchronously
- 🎬 **State Machine**: Xử lý events theo state
- ⚡ **Non-blocking**: Post event rồi return ngay

### 3. **State Handlers**
```c
/* Mỗi state là một hàm handler */
static StateHandler_t State_Running(ActiveObject_t *me, const Event_t *e)
{
    switch(e->sig) {
        case MOTOR_SPEED_UPDATE_SIG:
            me->speed_cmd = e->data.i32;
            RunMotor(me);
            break;
        case MOTOR_OVERCURRENT_SIG:
            return State_Error;  /* Transition: Running → Error */
        case EXIT_SIG:
            StopMotor(me);       /* Exit action */
            break;
    }
    return NULL;  /* Stay in Running state */
}
```

### 4. **Run-To-Completion (RTC)**
```
Dispatcher xử lý 1 event XONG mới xử lý event tiếp theo
↓
Không có race condition trong AO (nhưng có thể race giữa AOs)
↓
Không cần mutex - SAFE!

Timeline:
---------
Time 0: Dispatcher nhận SPEED_UPDATE → gọi State_Running()
Time 1:   State_Running nghĩ, chạy motor (RTC step 1 xong)
Time 2: Dispatcher nhận OVERCURRENT_SIG → gọi State_Running()
Time 3:   State_Running return State_Error (transition)
Time 4: Dispatcher gọi EXIT_SIG on Running, ENTRY_SIG on Error
Time 5: Idle - đợi events mới
```

### 5. **Scheduler (Cooperative Kernel QV)**
```c
void Scheduler_Run(Scheduler_t *sched)
{
    while(1) {
        /* Tìm AO có priority cao nhất (số nhỏ nhất) & có events */
        AO_with_highest_priority = FindHighestPriorityAO();
        
        if (AO_with_highest_priority) {
            /* Dispatch 1 RTC step */
            Dispatch(AO_with_highest_priority);
        } else {
            /* Tất cả queue trống - call idle callback */
            idle_callback();  /* Enter sleep mode */
        }
    }
}
```

---

## 📂 File Structure

```
middleware/event_framework/
├── event.h                      /* Core event definitions */
├── event.c                      /* Event queue implementation */
├── active_object.h              /* Base AO class + Scheduler */
├── active_object.c              /* AO & Scheduler implementation */
├── motor_controller_ao.h        /* Motor-specific AO */
└── motor_controller_ao.c        /* Motor AO implementation */

src/f4/
├── main.c                       /* CŨNG: bare-metal polling (giữ để reference) */
└── main_event_driven.c          /* MỚI: event-driven (dùng cái này) */
```

---

## 🚀 How to Use in Your Code

### Step 1: Khởi Tạo Scheduler & AOs
```c
int main(void)
{
    /* Hardware init */
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000U);
    BSP_Motor_Init();
    ADC_Init();
    
    /* Framework init */
    Scheduler_Init(&g_scheduler, idle_callback);
    
    /* Tạo Motor AOs */
    g_motor_a_ao = MotorController_Create(0, 1);  /* Channel 0, priority 1 */
    g_motor_b_ao = MotorController_Create(1, 1);
    
    /* Đăng ký vào scheduler */
    Scheduler_Register(&g_scheduler, g_motor_a_ao);
    Scheduler_Register(&g_scheduler, g_motor_b_ao);
    
    /* Start initial states */
    ActiveObject_Start(g_motor_a_ao);
    ActiveObject_Start(g_motor_b_ao);
    
    /* Main event loop */
    while(1) {
        UpdateSensors();           /* Tạo events từ sensors */
        Scheduler_Step(&g_scheduler); /* Process 1 RTC step */
    }
}
```

### Step 2: Post Events từ Sensors (không blocking!)
```c
void UpdateSensorsAO(void)
{
    if (time_for_sampling()) {
        uint16_t speed = ADC_ReadSpeed();
        uint16_t current = ADC_ReadCurrent();
        uint16_t temp = ADC_ReadTemp();
        
        /* Gửi events - đều asynchronous */
        MotorController_UpdateSpeed(motor_a_ao, speed);
        MotorController_UpdateSensors(motor_a_ao, current, temp);
        
        /* Hàm này return ngay - không chờ motor xử lý! */
    }
}
```

### Step 3: State Machine xử lý Events (thread-safe!)
```c
static StateHandler_t State_Running(ActiveObject_t *me, const Event_t *e)
{
    MotorControllerData_t *data = GetData(me);
    
    switch(e->sig) {
        case MOTOR_SPEED_UPDATE_SIG:
            data->speed_cmd = e->data.i32;
            RunMotor(data);
            /* Xử lý RTC step xong, quay về dispatcher */
            break;
            
        case SENSOR_UPDATE_SIG:
            data->current_ma = e->data.u16[0];
            data->temp_c = e->data.u16[1];
            
            /* Safety check */
            if (data->current_ma > data->max_current_ma) {
                return State_Error;  /* Transition */
            }
            break;
    }
    return NULL;  /* Stay in Running */
}
```

---

## 📊 Comparison: Bare-Metal vs Event-Driven

### Code in Main Loop:

**BEFORE (Bare-Metal):**
```c
while(1) {
    if ((now - last_tick) >= 100) {
        speed = ADC_Read();
        if (speed > 3500) Motor_Stop();
        else Motor_Run(speed);
        UART_Send();
        last_tick = now;
    }
}
```
- ❌ All logic in 1 loop
- ❌ Block on timers
- ❌ Hard to test
- ❌ Hard to add features

**AFTER (Event-Driven):**
```c
while(1) {
    UpdateSensorsAO();      /* Post events only */
    Scheduler_Step();       /* Process 1 RTC */
}
```
- ✅ Separation of concerns
- ✅ Non-blocking
- ✅ Easy to test (mock events)
- ✅ Scalable (add new events easily)

---

## 🔄 Event Flow Example

```
Timeline: motor speed ramp-up & safety check
══════════════════════════════════════════════

Time 0:
  └─ UpdateSensorsAO() → post MOTOR_SPEED_UPDATE(50) to motor_a_ao

Time 1:
  └─ Scheduler finds motor_a_ao has events
     └─ Dispatch: State_Ready(MOTOR_SPEED_UPDATE)
        └─ data->speed_cmd = 50
        └─ RunMotor(data)  ← Motor running at 50%
        └─ Transition: return State_Running

Time 2:
  └─ Scheduler delivers EXIT_SIG to State_Ready
  └─ Then ENTRY_SIG to State_Running
  └─ RTC step complete

Time 3:
  └─ UpdateSensorsAO() → post SENSOR_UPDATE(4000mA, 90°C) 
     └─ Current = 4000 > max (3500) → OVERCURRENT!

Time 4:
  └─ Dispatcher finds motor_a_ao has events
     └─ Dispatch: State_Running(SENSOR_UPDATE)
        └─ Check currrent > max → ERROR!
        └─ Transition: return State_Error

Time 5:
  └─ Exit State_Running → StopMotor()
  └─ Enter State_Error → Emergency stop + log

Result:
  ✓ Motor stopped safely
  ✓ No blocking anywhere
  ✓ All events processed in RTC order
  ✓ State machine ensures safety
```

---

## 🎯 Best Practices

### 1. **Mỗi AO có dữ liệu private**
```c
/* GOOD */
typedef struct {
    int16_t speed_cmd;
    uint16_t current_ma;
} MotorData_t;

MotorData_t *data = (MotorData_t *)ao->priv;

/* BAD - shared global state → race conditions */
static int16_t g_speed = 0;  /* ✗ NO! */
```

### 2. **State handlers KHÔNG block**
```c
/* GOOD - return immediately */
static StateHandler_t State_Running(AO *me, const Event_t *e)
{
    data->speed = e->data.i32;
    RunMotor(data);
    return NULL;  /* RTC step complete */
}

/* BAD - blocking in handler */
static StateHandler_t State_Running(AO *me, const Event_t *e)
{
    delay_ms(100);  /* ✗ NEVER! Blocks entire scheduler */
    RunMotor(data);
}
```

### 3. **Post events from ISRs**
```c
/* ISR context - lightweight & non-blocking */
void UART_RxISR(void)
{
    Event_t e = Event_Create(UART_RX_SIG);
    ActiveObject_Post(comm_ao, &e);  /* Queue event - return immediately */
}

/* ✓ NO race condition - just queuing
   ✓ Main loop handles event later
   ✓ ISR not delayed by processing
```

### 4. **Initialize AOs before using**
```c
ActiveObject_Init(ao, priority, initial_state, private_data);
ActiveObject_Start(ao);  /* Send ENTRY_SIG */

/* ✓ AO ready to receive events */
```

---

## 🧪 Testing with Event-Driven

**Advantage: Mockable events!**

```c
/* Unit test - inject events */
void test_motor_overcurrent(void)
{
    ActiveObject_t *ao = MotorController_Create(0, 1);
    ActiveObject_Start(ao);
    
    /* Simulate: Motor running */
    MotorController_Start(ao, 100);
    ActiveObject_Dispatch(ao);  /* Process start event */
    
    /* Simulate: Overcurrent detected */
    MotorController_UpdateSensors(ao, 4000, 25);  /* 4000 > 3500 */
    ActiveObject_Dispatch(ao);  /* Process sensor event */
    
    /* Verify: Motor in ERROR state */
    assert(MotorController_GetState(ao) == MC_STATE_ERROR);
    
    MotorController_Destroy(ao);
}
```

---

## 📝 Migration Checklist

- [ ] Copy `middleware/event_framework/` files
- [ ] Update project build system to include new files
- [ ] Replace `main.c` with `main_event_driven.c`
- [ ] Test scheduler & AOs initialization
- [ ] Verify events posted correctly
- [ ] Check state transitions
- [ ] Monitor stack usage (should be lower!)
- [ ] Test safety limits (overcurrent, overheat)
- [ ] Implement idle callback (sleep mode)

---

## 📚 References

- **PDF**: Beyond the RTOS (Quantum Leaps presentation)
- **Framework**: Inspired by QP/C active object framework
- **Pattern**: Active Object / Actor design pattern

---

## ✅ Summary

| Aspect | Bare-Metal | RTOS | Event-Driven |
|--------|-----------|------|--------------|
| **Blocking** | ❌ Poll loop | ⚠️ Mutex/Sem | ✅ None |
| **Stack usage** | ✅ Minimal | ❌ Per-thread | ✅ Minimal |
| **Race conditions** | ❌ Shared state | ⚠️ Complex | ✅ Encapsulated |
| **Scalability** | ❌ Spaghetti | ⚠️ Medium | ✅ Excellent |
| **Testing** | ⚠️ Hard | ⚠️ Hard | ✅ Easy (mock events) |
| **Responsive** | ❌ Polling delays | ✅ High | ✅ Very high |
| **Learning curve** | ✅ Easy | ❌ Hard | ✅ Medium |

**Kết luận**: Kiến trúc event-driven là **tối ưu cho embedded real-time systems**! 🎉


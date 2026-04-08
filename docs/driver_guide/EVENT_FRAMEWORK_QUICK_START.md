/**
 * @file QUICK_START.md
 * @brief Event-Driven Framework - Quick Start Guide
 * @date 2026-04-01
 */

# 🚀 Event-Driven Framework - Quick Start

## 👀 5-Minute Overview

**What changed?**
```c
// BEFORE: Bare-metal polling
while(1) {
    speed = ADC_ReadSpeed();
    Motor_Run(speed);
}

// AFTER: Event-driven
MotorController_UpdateSpeed(motor_ao, speed);
Scheduler_Step(&scheduler);
```

**Key principle**: Post events → Scheduler processes → State machine reacts

---

## 📦 3 Main Components

### 1. **Events** - Messages between AOs
```c
Event_t e = Event_CreateWithI32(MOTOR_SPEED_UPDATE_SIG, 75);
// Meaning: "Update motor speed to 75%"
```

### 2. **Active Objects** - Encapsulated entities with state machine
```c
ActiveObject_t *motor_ao = MotorController_Create(0, 1);
ActiveObject_Start(motor_ao);
// Now motor_ao is ready to receive events
```

### 3. **Scheduler** - Coordinates AOs
```c
Scheduler_Register(&scheduler, motor_ao);
Scheduler_Step(&scheduler);  // Process 1 event
```

---

## 🔧 Minimal Example

```c
#include "event.h"
#include "active_object.h"
#include "motor_controller_ao.h"

int main(void)
{
    /* 1. Init hardware */
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000U);
    BSP_Motor_Init();
    ADC_Init();
    
    /* 2. Create scheduler */
    Scheduler_t scheduler;
    Scheduler_Init(&scheduler, NULL);
    
    /* 3. Create motor AO */
    ActiveObject_t *motor = MotorController_Create(0, 1);
    Scheduler_Register(&scheduler, motor);
    ActiveObject_Start(motor);
    
    /* 4. Send events & process */
    while(1) {
        /* Read sensor & post event */
        uint16_t speed = ADC_ReadSpeed();
        MotorController_UpdateSpeed(motor, speed);
        
        /* Process 1 RTC step */
        Scheduler_Step(&scheduler);
    }
}
```

**That's it!** No threads, no semaphores, no blocking. 🎉

---

## 📋 Common Tasks

### Task 1: Create a New Active Object

```c
/* 1. Define private data */
typedef struct {
    uint16_t value;
    uint32_t timestamp;
} MyPrivateData_t;

/* 2. Define state handlers */
static StateHandler_t State_Init(ActiveObject_t *me, const Event_t *e);
static StateHandler_t State_Active(ActiveObject_t *me, const Event_t *e);

/* 3. Implement state handlers */
static StateHandler_t State_Init(ActiveObject_t *me, const Event_t *e)
{
    MyPrivateData_t *data = (MyPrivateData_t *)me->priv;
    
    switch(e->sig) {
        case ENTRY_SIG:
            data->value = 0;
            break;
        case INIT_SIG:
            return State_Active;  /* Transition */
    }
    return NULL;
}

static StateHandler_t State_Active(ActiveObject_t *me, const Event_t *e)
{
    MyPrivateData_t *data = (MyPrivateData_t *)me->priv;
    
    switch(e->sig) {
        case ENTRY_SIG:
            data->timestamp = app_millis();
            break;
        case USER_EVENT:
            data->value = e->data.u32;
            break;
    }
    return NULL;
}

/* 4. Create factory function */
ActiveObject_t* MyObject_Create(uint8_t priority)
{
    ActiveObject_t *ao = malloc(sizeof(*ao));
    MyPrivateData_t *data = malloc(sizeof(*data));
    
    ActiveObject_Init(ao, priority, State_Init, data);
    return ao;
}
```

### Task 2: Post Events from Different Places

**From main loop:**
```c
MotorController_UpdateSpeed(motor_ao, 50);
```

**From ISR:**
```c
void UART_RxISR(void)
{
    uint8_t byte = UART_GetByte();
    Event_t e = Event_CreateWithU32(UART_RX_SIG, byte);
    ActiveObject_Post(comm_ao, &e);  /* Non-blocking - just queue */
}
```

**From another AO:**
```c
static StateHandler_t State_Processing(ActiveObject_t *me, const Event_t *e)
{
    if (e->sig == PROCESS_EVENT) {
        /* Post event to another AO */
        Event_t result_e = Event_CreateWithU32(RESULT_READY_SIG, 123);
        ActiveObject_Post(other_ao, &result_e);
    }
    return NULL;
}
```

### Task 3: Handle State Transitions

```c
static StateHandler_t State_Running(ActiveObject_t *me, const Event_t *e)
{
    switch(e->sig) {
        case ENTRY_SIG:
            printf("  → Entered Running state\n");
            break;
            
        case STOP_EVENT:
            printf("  ← Exiting Running state\n");
            return State_Idle;  /* Next state handler */
            
        case EXIT_SIG:
            /* Cleanup before exiting */
            break;
    }
    return NULL;  /* Stay in current state */
}
```

**When transition happens:**
1. `EXIT_SIG` → State_Running (exit action)
2. State changes: `me->state = State_Idle`
3. `ENTRY_SIG` → State_Idle (entry action)

### Task 4: Access Private Data

```c
/* In state handler - direct access */
MotorControllerData_t *data = (MotorControllerData_t *)me->priv;

/* From outside - use getter */
MotorControllerData_t *data = MotorController_GetData(motor_ao);
printf("Speed: %d\n", data->speed_cmd);
```

### Task 5: Monitor Queue Status

```c
/* Check if AO has pending events */
if (ActiveObject_HasEvents(motor_ao)) {
    printf("Motor AO has events pending\n");
}

/* Get count */
uint16_t count = ActiveObject_EventCount(motor_ao);
printf("Events in queue: %d\n", count);
```

---

## 🎬 Event Types You'll Use

```c
/* Control events */
MOTOR_START_SIG,           /* Start motor */
MOTOR_STOP_SIG,            /* Stop motor */
MOTOR_SPEED_UPDATE_SIG,    /* Update speed */

/* Safety events */
MOTOR_OVERCURRENT_SIG,     /* Current too high */
MOTOR_OVERHEAT_SIG,        /* Temperature too high */

/* Sensor events */
SENSOR_UPDATE_SIG,         /* New sensor data */
SENSOR_READ_SIG,           /* Request to read */

/* Timer events */
TICK_SIG,                  /* Periodic tick */
TIMEOUT_SIG,               /* Timeout expired */

/* Communication events */
UART_RX_SIG,               /* UART data received */
UART_TX_SIG,               /* UART data to send */
```

---

## ⚡ Performance Tips

### 1. Event Queue Size
```c
#define EVENT_QUEUE_SIZE 16  /* Increase if queue overflows */
```

### 2. Priority Levels
```c
/* Lower number = higher priority */
MotorController_Create(0, 1);   /* Priority 1 - high */
SensorSampler_Create(2);        /* Priority 2 - medium */
Logger_Create(3);               /* Priority 3 - low */
```

### 3. Idle Callback (Power Saving)
```c
static void idle_callback(void)
{
    /* CPU enters sleep/low-power mode */
    SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;
    __WFI();  /* Wait For Interrupt */
}

Scheduler_Init(&scheduler, idle_callback);
```

### 4. Stack Check
```c
/* Event-driven uses LESS stack than RTOS */
/* No per-thread stacks - only one call stack */
/* All AOs share same stack - much more efficient */
```

---

## 🐛 Debugging Tips

### Print Event Flow
```c
void DebugPrintEvent(const Event_t *e)
{
    printf("Event: sig=%d, data.u32=%d\n", e->sig, e->data.u32);
}
```

### Monitor State Changes
```c
MotorControllerData_t *data = MotorController_GetData(motor_ao);
printf("Motor state: %d, speed=%d, current=%dmA\n",
       data->state, data->speed_cmd, data->current_ma);
```

### Check Queue Status
```c
if (ActiveObject_EventCount(motor_ao) > 10) {
    printf("WARNING: Motor queue size=%d (might overflow)\n",
           ActiveObject_EventCount(motor_ao));
}
```

---

## ❌ Common Mistakes

### ❌ Mistake 1: Blocking in state handler
```c
/* WRONG */
static StateHandler_t State_Running(ActiveObject_t *me, const Event_t *e)
{
    delay_ms(100);  /* ✗ NO! Blocks entire scheduler */
    RunMotor(data);
}

/* RIGHT */
static StateHandler_t State_Running(ActiveObject_t *me, const Event_t *e)
{
    /* Just set data & return - dispatcher will call again later */
    RunMotor(data);
    return NULL;  /* RTC step done immediately */
}
```

### ❌ Mistake 2: Sharing state between AOs without events
```c
/* WRONG */
static int16_t g_shared_speed = 0;
motor_ao->priv = &g_shared_speed;
other_ao->priv = &g_shared_speed;  /* Race condition! */

/* RIGHT */
motor_ao->priv = malloc(sizeof(MotorData_t));
other_ao->priv = malloc(sizeof(OtherData_t));
/* Communicate via events only */
```

### ❌ Mistake 3: Forgetting to post initial START event
```c
/* WRONG */
ActiveObject_Start(motor_ao);
/* AO is now in initial state but has no work to do */

/* RIGHT */
ActiveObject_Start(motor_ao);
MotorController_Start(motor_ao, 0);  /* Post initial command */
```

---

## 🔄 State Machine Template

```c
/* Define states */
static StateHandler_t State_Idle(ActiveObject_t *me, const Event_t *e);
static StateHandler_t State_Active(ActiveObject_t *me, const Event_t *e);

/* State Idle */
static StateHandler_t State_Idle(ActiveObject_t *me, const Event_t *e)
{
    switch(e->sig) {
        case ENTRY_SIG:
            printf("Entering Idle\n");
            /* Setup idle state */
            break;
            
        case START_EVENT:
            /* Do something */
            return State_Active;  /* Transition to Active */
            
        case EXIT_SIG:
            printf("Exiting Idle\n");
            /* Cleanup */
            break;
            
        default:
            break;
    }
    return NULL;  /* Stay in Idle */
}

/* State Active */
static StateHandler_t State_Active(ActiveObject_t *me, const Event_t *e)
{
    switch(e->sig) {
        case ENTRY_SIG:
            printf("Entering Active\n");
            break;
            
        case PROCESS_EVENT:
            /* Handle event */
            break;
            
        case STOP_EVENT:
            return State_Idle;  /* Go back to Idle */
            
        case EXIT_SIG:
            printf("Exiting Active\n");
            break;
            
        default:
            break;
    }
    return NULL;  /* Stay in Active */
}
```

---

## 📊 Comparison: Before vs After

| Feature | Before (Polling) | After (Events) |
|---------|-----------------|----------------|
| **Response time** | 100ms polling → slow | Immediate → fast ⚡ |
| **Stack usage** | Shared (risky) | Shared (safe) ✅ |
| **Testing** | Hard (inject timers) | Easy (inject events) ✅ |
| **Adding features** | Modify main loop | Add new AO ✅ |
| **Race conditions** | Global state | Encapsulated ✅ |
| **Code clarity** | Linear but tangled | State machine clear ✅ |

---

## ✅ Next Steps

1. ✅ Copy framework files
2. ✅ Replace main.c with main_event_driven.c  
3. ✅ Build & test
4. ✅ Create new AOs for your features
5. ✅ Post events from sensors/ISRs
6. ✅ Enjoy better architecture! 🎉

---

## 📚 Documentation Files

- `EVENT_DRIVEN_GUIDE.md` - Detailed architecture guide
- `QUICK_START.md` - This file
- `event.h` - Event definitions & queue API
- `active_object.h` - AO base class & scheduler
- `motor_controller_ao.h` - Example: Motor AO

---

## 💡 Pro Tips

1. **Use tool like CANdb++ or UML2Code** to generate state machines
2. **Add debug tracing** - hook into Scheduler_Step to log events
3. **Implement timeout events** - use SysTick to post TICK_SIG periodically
4. **Test state machine** - inject events one by one, verify transitions
5. **Monitor the queue** - ensure queue never overflows

---

**Happy event-driven coding!** 🚀


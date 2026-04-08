/**
 * @file led_fsm.c
 * @brief LED Running Light FSM Implementation (Non-blocking)
 * @description
 *   Demonstrates FSM applied to LED control:
 *   - 3 States: IDLE, ON_SEQUENCE, OFF_SEQUENCE
 *   - Event-driven state transitions
 *   - Non-blocking timer (elimates blocking delays)
 *   - Can be called repeatedly in a loop without blocking
 */

#include <stdint.h>
#include <stdio.h>

/* ============================================================================
 * FSM TYPE DEFINITIONS
 * ============================================================================ */

/**
 * LED FSM States
 * - IDLE: Waiting for next sequence to start
 * - ON_SEQUENCE: Turning LEDs on sequentially (L0->L7)
 * - OFF_SEQUENCE: Turning LEDs off sequentially (L0->L7)
 */
typedef enum {
    LED_STATE_IDLE,
    LED_STATE_ON_SEQUENCE,
    LED_STATE_OFF_SEQUENCE
} led_fsm_state_t;

/**
 * LED FSM Events
 * - START: Trigger next sequence
 * - TIMEOUT: Timer expired, move to next LED
 * - SEQUENCE_DONE: All LEDs in current phase done
 */
typedef enum {
    LED_EVENT_START,
    LED_EVENT_TIMEOUT,
    LED_EVENT_SEQUENCE_DONE
} led_fsm_event_t;

/**
 * LED FSM Context (State machine instance)
 * Holds:
 * - Current state
 * - Current LED index (0-7)
 * - Timer counter (in milliseconds)
 */
typedef struct {
    led_fsm_state_t state;
    uint8_t led_index;         /* 0-7, which LED in current sequence */
    uint32_t timer_ms;         /* Timer counter */
    uint32_t timer_interval;   /* Interval for LED delay (500ms) */
} led_fsm_t;

/* ============================================================================
 * REGISTER DEFINITIONS (STM32F103)
 * ============================================================================ */

#define RCC_BASE            0x40021000UL
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x18))

#define GPIOB_BASE          0x40010C00UL
#define GPIOB_CRL           (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_CRH           (*(volatile uint32_t *)(GPIOB_BASE + 0x04))
#define GPIOB_ODR           (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

#define LED_COUNT           8       /* PB0-PB7 = 8 LEDs */
#define LED_DELAY_MS        500     /* 500ms between each LED */

/* ============================================================================
 * FORWARD DECLARATIONS (from uart2_driver.c)
 * ============================================================================ */

void UART2_SendString(const char *str);
void UART2_Printf(const char *format, ...);

/* ============================================================================
 * GPIO INITIALIZATION (same as original)
 * ============================================================================ */

void GPIO_Init(void)
{
    /* Enable GPIOB clock (APB2) */
    RCC_APB2ENR |= (1 << 3);  /* GPIOB clock enable */
    
    /* Configure PB0-PB7 as outputs */
    GPIOB_CRL &= 0x00000000;
    GPIOB_CRL |= 0x11111111;  /* Set PB0-PB7 all outputs (mode=01) */
    
    /* Turn all LEDs off initially */
    GPIOB_ODR = 0x00;
}

/* ============================================================================
 * LED CONTROL PRIMITIVES
 * ============================================================================ */

void LED_On(uint8_t led_num)
{
    if (led_num < LED_COUNT) {
        GPIOB_ODR |= (1 << led_num);
    }
}

void LED_Off(uint8_t led_num)
{
    if (led_num < LED_COUNT) {
        GPIOB_ODR &= ~(1 << led_num);
    }
}

/* ============================================================================
 * FSM IMPLEMENTATION
 * ============================================================================ */

/**
 * Initialize FSM context
 */
void LED_FSM_Init(led_fsm_t *fsm)
{
    fsm->state = LED_STATE_IDLE;
    fsm->led_index = 0;
    fsm->timer_ms = 0;
    fsm->timer_interval = LED_DELAY_MS;
    UART2_SendString("[FSM] LED FSM Initialized\r\n");
}

/**
 * Generate events based on internal timers and conditions
 * Call this function periodically (e.g., every 1ms from a timer ISR or RTOS tick)
 */
led_fsm_event_t LED_FSM_GetEvent(led_fsm_t *fsm)
{
    /* Check if timer has expired */
    if (fsm->timer_ms >= fsm->timer_interval) {
        /* Reset timer */
        fsm->timer_ms = 0;
        
        /* Check if all LEDs in sequence are done */
        if (fsm->led_index >= LED_COUNT - 1) {
            return LED_EVENT_SEQUENCE_DONE;
        }
        
        return LED_EVENT_TIMEOUT;
    }
    
    return (led_fsm_event_t)-1;  /* No event */
}

/**
 * Main FSM state machine logic
 * This is where state transitions happen
 */
void LED_FSM_Execute(led_fsm_t *fsm)
{
    /* Get the current event */
    led_fsm_event_t event = LED_FSM_GetEvent(fsm);
    
    /* Handle no event case */
    if ((int)event == -1) {
        return;
    }

    /* =========== STATE MACHINE LOGIC =========== */
    switch (fsm->state) {
        
        /* --------- IDLE STATE --------- */
        case LED_STATE_IDLE:
            if (event == LED_EVENT_START) {
                /* Transition to ON_SEQUENCE */
                fsm->state = LED_STATE_ON_SEQUENCE;
                fsm->led_index = 0;
                fsm->timer_ms = 0;
                UART2_SendString("[FSM] IDLE -> ON_SEQUENCE\r\n");
                LED_On(fsm->led_index);
                UART2_Printf("[FSM] LED%d ON\r\n", fsm->led_index);
            }
            break;
        
        /* --------- ON_SEQUENCE STATE --------- */
        case LED_STATE_ON_SEQUENCE:
            if (event == LED_EVENT_TIMEOUT) {
                /* Move to next LED */
                fsm->led_index++;
                LED_On(fsm->led_index);
                UART2_Printf("[FSM] LED%d ON\r\n", fsm->led_index);
            }
            else if (event == LED_EVENT_SEQUENCE_DONE) {
                /* All LEDs turned on, transition to OFF_SEQUENCE */
                fsm->state = LED_STATE_OFF_SEQUENCE;
                fsm->led_index = 0;
                fsm->timer_ms = 0;
                UART2_SendString("[FSM] ON_SEQUENCE -> OFF_SEQUENCE\r\n");
                LED_Off(fsm->led_index);
                UART2_Printf("[FSM] LED%d OFF\r\n", fsm->led_index);
            }
            break;
        
        /* --------- OFF_SEQUENCE STATE --------- */
        case LED_STATE_OFF_SEQUENCE:
            if (event == LED_EVENT_TIMEOUT) {
                /* Move to next LED */
                fsm->led_index++;
                LED_Off(fsm->led_index);
                UART2_Printf("[FSM] LED%d OFF\r\n", fsm->led_index);
            }
            else if (event == LED_EVENT_SEQUENCE_DONE) {
                /* All LEDs turned off, transition back to IDLE */
                fsm->state = LED_STATE_IDLE;
                fsm->led_index = 0;
                fsm->timer_ms = 0;
                UART2_SendString("[FSM] OFF_SEQUENCE -> IDLE\r\n");
                UART2_SendString("[FSM] === One complete cycle done ===\r\n");
            }
            break;
        
        default:
            break;
    }
}

/**
 * Update FSM timer (call this every 1ms from a timer or RTOS tick)
 */
void LED_FSM_Update(led_fsm_t *fsm, uint32_t elapsed_ms)
{
    fsm->timer_ms += elapsed_ms;
}

/**
 * Get current state as string (for debugging)
 */
const char* LED_FSM_StateToString(led_fsm_t *fsm)
{
    switch (fsm->state) {
        case LED_STATE_IDLE:        return "IDLE";
        case LED_STATE_ON_SEQUENCE: return "ON_SEQUENCE";
        case LED_STATE_OFF_SEQUENCE: return "OFF_SEQUENCE";
        default:                    return "UNKNOWN";
    }
}

/* ============================================================================
 * SIMPLE TIMER ISR SIMULATION (for testing without RTOS)
 * ============================================================================ */

/** Simple SysTick-like timer (call every 1ms) */
static uint32_t system_tick_ms = 0;

void SysTickHandler(void)
{
    system_tick_ms++;
}

uint32_t GetTick(void)
{
    return system_tick_ms;
}

/* ============================================================================
 * MAIN TEST LOOP
 * ============================================================================ */

int main(void)
{
    /* Initialize peripherals */
    GPIO_Init();
    UART2_Init();
    
    UART2_SendString("\r\n========================================\r\n");
    UART2_SendString("[BOOT] LED FSM Test Started\r\n");
    UART2_SendString("========================================\r\n\r\n");
    
    /* Create FSM instance */
    led_fsm_t led_fsm;
    LED_FSM_Init(&led_fsm);
    
    UART2_SendString("\r\n[INFO] Starting LED sequences...\r\n");
    
    /* Main loop - simulate a 1ms tick-based system */
    uint32_t last_tick = 0;
    uint32_t cycle_count = 0;
    
    while (1) {
        uint32_t current_tick = GetTick();  /* Get "current time" */
        
        /* Simulate 1ms tick */
        if (current_tick != last_tick) {
            last_tick = current_tick;
            
            /* Update FSM timer */
            LED_FSM_Update(&led_fsm, 1);  /* +1ms elapsed */
            
            /* Execute FSM (no blocking!) */
            LED_FSM_Execute(&led_fsm);
            
            /* Kick-start the FSM on first tick */
            if (current_tick == 1) {
                UART2_SendString("[MAIN] Triggering LED_EVENT_START\r\n");
                led_fsm.state = LED_STATE_IDLE;  /* Force START event handling */
            }
        }
        
        /* Check if one complete cycle done */
        if (led_fsm.state == LED_STATE_IDLE && current_tick > 100) {
            cycle_count++;
            UART2_Printf("[MAIN] Cycle #%lu complete\r\n", cycle_count);
            
            /* Restart */
            LED_FSM_Init(&led_fsm);
            
            if (cycle_count >= 3) {
                UART2_SendString("\r\n[DEBUG] Test completed 3 cycles\r\n");
                break;  /* Exit after 3 cycles for demo */
            }
        }
    }
    
    return 0;
}

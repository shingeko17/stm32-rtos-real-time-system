/**
 * @file watchdog_main.c
 * @brief STM32F103 Backup MCU - Watchdog & Failover Controller
 * @date 2026-03-29
 *
 * Functionality:
 * - Listen for heartbeat from F4 via USART2 (PA3 RX)
 * - Detect F4 crash (200ms timeout = 2+ missed 0xAA pulses)
 * - Activate failover motor control (coast when alive, coast when dead)
 * - Hardware IWDG monitors F1 itself (1s timeout)
 * - LED status (PC13): ON=F4 OK, OFF=F4 DEAD
 *
 * Register-level implementation (no vendor library needed).
 */

#include <stdint.h>


/* ============================================================================
 * REGISTER BASE ADDRESSES
 * ============================================================================ */

#define RCC_BASE              0x40021000UL
#define RCC_CR                (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR              (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_APB2ENR           (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_APB1ENR           (*(volatile uint32_t *)(RCC_BASE + 0x1C))

#define RCC_CR_HSEON          (1 << 16)
#define RCC_CR_HSERDY         (1 << 17)

#define GPIOC_BASE            0x40011000UL
#define GPIOC_CRH             (*(volatile uint32_t *)(GPIOC_BASE + 0x04))
#define GPIOC_ODR             (*(volatile uint32_t *)(GPIOC_BASE + 0x0C))

#define GPIOA_BASE            0x40010800UL
#define GPIOA_CRL             (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_CRH             (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#define GPIOA_ODR             (*(volatile uint32_t *)(GPIOA_BASE + 0x0C))

#define USART2_BASE           0x40004400UL
#define USART2_SR             (*(volatile uint32_t *)(USART2_BASE + 0x00))
#define USART2_DR             (*(volatile uint32_t *)(USART2_BASE + 0x04))
#define USART2_BRR            (*(volatile uint32_t *)(USART2_BASE + 0x08))
#define USART2_CR1            (*(volatile uint32_t *)(USART2_BASE + 0x0C))
#define USART2_CR2            (*(volatile uint32_t *)(USART2_BASE + 0x10))
#define USART2_CR3            (*(volatile uint32_t *)(USART2_BASE + 0x14))

#define SysTick_BASE          0xE000E010UL
#define SysTick_CTRL          (*(volatile uint32_t *)(SysTick_BASE + 0x00))
#define SysTick_LOAD          (*(volatile uint32_t *)(SysTick_BASE + 0x04))
#define SysTick_VAL           (*(volatile uint32_t *)(SysTick_BASE + 0x08))

#define IWDG_BASE             0x40003000UL
#define IWDG_KR               (*(volatile uint32_t *)(IWDG_BASE + 0x00))
#define IWDG_PR               (*(volatile uint32_t *)(IWDG_BASE + 0x04))
#define IWDG_RLR              (*(volatile uint32_t *)(IWDG_BASE + 0x08))
#define IWDG_SR               (*(volatile uint32_t *)(IWDG_BASE + 0x0C))

#define NVIC_ISER             (*(volatile uint32_t *)(0xE000E100UL))
#define NVIC_ICER             (*(volatile uint32_t *)(0xE000E180UL))
#define NVIC_IPR              ((volatile uint32_t *)(0xE000E400UL))

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

#define HEARTBEAT_TIMEOUT_MS  200
#define HEARTBEAT_PULSE       0xAA
#define LED_TOGGLE_MS         500

#define IWDG_KEY_FEED         0xAAAAUL
#define IWDG_KEY_UNLOCK       0x5555UL
#define IWDG_KEY_START        0xCCCCUL
#define IWDG_PR_DIV64         0x04U
#define IWDG_RELOAD_1S        625U

/* USART2 Baud rate: 115200 @ 36MHz APB1
 * BRR = APB1_CLOCK / (16 * BAUD)
 * BRR = 36000000 / (16 * 115200) = 19.53 ≈ 20 (0x14)
 */
#define USART2_BAUD_RATE      115200


/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================ */

static volatile uint32_t g_heartbeat_timer = 0;
static volatile uint8_t g_f4_alive = 1;
static volatile uint32_t g_system_ticks = 0;

/* ============================================================================
 * SYSTEM TICK HANDLER (1ms interrupt)
 * ============================================================================ */

void SysTick_Handler(void)
{
    g_system_ticks++;

    if (g_heartbeat_timer > 0) {
        g_heartbeat_timer--;
    } else {
        /* Timeout: F4 not sending heartbeat */
        if (g_f4_alive) {
            g_f4_alive = 0;  /* Mark F4 as dead */
        }
    }
}

/* ============================================================================
 * UART2 INTERRUPT HANDLER
 * ============================================================================ */

void USART2_IRQHandler(void)
{
    if (USART2_SR & 0x20) {  /* RXNE flag */
        uint8_t data = USART2_DR & 0xFF;

        if (data == HEARTBEAT_PULSE) {
            /* Heartbeat 0xAA received from F4 */
            g_heartbeat_timer = HEARTBEAT_TIMEOUT_MS;

            /* If was dead, now alive */
            if (!g_f4_alive) {
                g_f4_alive = 1;
            }
        }

        USART2_SR = USART2_SR & ~0x20;  /* Clear RXNE */
    }
}

/* ============================================================================
 * CLOCK INITIALIZATION
 * ============================================================================ */

void Clock_Init(void)
{
    /* Set clock to HSI 8MHz (internal clock, simple) */
    /* For safety and predictability - no HSE on all boards */
    RCC_CFGR &= ~0x03;      /* Clear SW bits */
    RCC_CFGR |= 0x00;       /* SW = 00 (HSI) - runs at ~8MHz default */
}

/* ============================================================================
 * GPIO INITIALIZATION
 * ============================================================================ */

void GPIO_Init(void)
{
    /* Enable GPIOC and GPIOA clocks */
    RCC_APB2ENR |= (1 << 4);  /* GPIOC */
    RCC_APB2ENR |= (1 << 2);  /* GPIOA */

    /* Configure PC13 as output (LED) - mode=3 (50MHz push-pull) */
    GPIOC_CRH &= ~(0xF << 20);
    GPIOC_CRH |= (0x3 << 20);
    GPIOC_ODR |= (1 << 13);  /* LED off initially */

    /* Configure PA2, PA3 for USART2
     * PA2 = RX (input floating)
     * PA3 = TX (alternate function push-pull)
     */
    GPIOA_CRL &= ~(0xF << 8);
    GPIOA_CRL |= (0x4 << 8);  /* PA2 input floating */

    GPIOA_CRL &= ~(0xF << 12);
    GPIOA_CRL |= (0xB << 12); /* PA3 alternate push-pull */
}

/* ============================================================================
 * UART2 INITIALIZATION
 * ============================================================================ */

void USART2_Init(void)
{
    /* Enable USART2 and GPIOA clocks */
    RCC_APB1ENR |= (1 << 17);  /* USART2 */
    RCC_APB2ENR |= (1 << 2);   /* GPIOA */

    /* Baud rate: 115200 @ 8MHz APB1
     * BRR = APB1_CLOCK / (16 * BAUD) = 8000000 / (16 * 115200) ≈ 4
     */
    USART2_BRR = 4;

    /* CR1: Enable RX, enable RX interrupt */
    USART2_CR1 = 0x0020;  /* RE=1 */
    USART2_CR1 |= 0x0020; /* RXNEIE=1 */

    /* CR2: 1 stop bit */
    USART2_CR2 = 0x0000;

    /* CR3: No flow control */
    USART2_CR3 = 0x0000;

    /* Enable USART2 */
    USART2_CR1 |= 0x2000;  /* UE=1 */

    /* Enable USART2_IRQn (USART2_IRQ = 38 = 6 in ISER0) in NVIC */
    NVIC_ISER = (1 << 6);
}

/* ============================================================================
 * SYSTICK INITIALIZATION (1ms tick)
 * ============================================================================ */

void SysTick_Init(void)
{
    /* Load: 8MHz / 1000 = 8000 ticks per ms */
    SysTick_LOAD = 8000 - 1;

    /* VAL: Clear current value */
    SysTick_VAL = 0;

    /* CTRL: enable SysTick + interrupt, use AHB clock */
    SysTick_CTRL = 0x07;  /* ENABLE=1, TICKINT=1, CLKSOURCE=1 */
}

/* ============================================================================
 * INDEPENDENT WATCHDOG (IWDG)
 * ============================================================================ */

void IWDG_Init(void)
{
    /* Unlock PR and RLR */
    IWDG_KR = IWDG_KEY_UNLOCK;

    /* Wait until PR is writable */
    while (IWDG_SR & 0x01) {}

    /* Prescaler /64: LSI 40kHz / 64 = 625Hz -> 1.6ms per tick */
    IWDG_PR = IWDG_PR_DIV64;

    /* Wait until RLR is writable */
    while (IWDG_SR & 0x02) {}

    /* Reload: 625 * 1.6ms = 1000ms */
    IWDG_RLR = IWDG_RELOAD_1S;

    /* Feed once */
    IWDG_KR = IWDG_KEY_FEED;

    /* Start IWDG (cannot be disabled after this) */
    IWDG_KR = IWDG_KEY_START;
}

/* ============================================================================
 * IWDG FEED (Kick watchdog)
 * ============================================================================ */

void IWDG_Feed(void)
{
    IWDG_KR = IWDG_KEY_FEED;
}

/* ============================================================================
 * LED CONTROL
 * ============================================================================ */

void LED_Update(void)
{
    if (g_f4_alive) {
        GPIOC_ODR |= (1 << 13);   /* LED on */
    } else {
        GPIOC_ODR &= ~(1 << 13);  /* LED off */
    }
}

/* ============================================================================
 * MOTOR CONTROL (FAILOVER)
 * ============================================================================ */

void Motor_Coast(void)
{
    /* All motor pins LOW -> coast mode */
    GPIOA_ODR &= ~(0x0F << 8);  /* PA8-PA11 all LOW */
}

void Motor_Brake(void)
{
    /* All motor pins HIGH -> brake mode */
    GPIOA_ODR |= (0x0F << 8);   /* PA8-PA11 all HIGH */
}

/* ============================================================================
 * MAIN APPLICATION
 * ============================================================================ */

int main(void)
{
    /* System initialization */
    Clock_Init();
    GPIO_Init();
    USART2_Init();
    SysTick_Init();

    /* Initialize heartbeat timeout */
    g_heartbeat_timer = HEARTBEAT_TIMEOUT_MS;
    g_f4_alive = 1;

    /* Start hardware watchdog (F1 resets if main loop stalls >1s) */
    IWDG_Init();

    /* Main superloop */
    while (1) {
        /* Kick IWDG every loop iteration (much sooner than 1s timeout) */
        IWDG_Feed();

        /* Update LED status every 500ms */
        if ((g_system_ticks % LED_TOGGLE_MS) == 0) {
            LED_Update();
        }

        /* Check F4 status and maintain safe motor state */
        if (g_f4_alive) {
            /* F4 is alive: coast motor */
            Motor_Coast();
        } else {
            /* F4 is dead: coast motor for safety */
            Motor_Coast();
        }
    }

    return 0;
}

/* ============================================================================
 * INTERRUPT VECTOR HOOKS (Weak defaults)
 * ============================================================================ */

void Default_Handler(void) { while (1); }

void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));

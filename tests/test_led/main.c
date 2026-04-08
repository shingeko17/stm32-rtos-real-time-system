/**
 * @file main.c
 * @brief STM32F103C8T6 LED Running Light (Chase Effect) Test with UART2
 * @date 2026-03-29
 *
 * LED mapping (PB0-PB7):
 * L0 = PB0 (LED0), L1 = PB1 (LED1), ..., L7 = PB7 (LED7)
 *
 * UART2 Output:
 * - PA2: TX (gửi LED status)
 * - PA3: RX (nhận commands từ F4)
 * - Baud: 9600
 *
 * Effect: Sáng dồn (ON L0->L7) + Tắt dồn (OFF L0->L7), delay 500ms mỗi LED
 * Logic: Active HIGH (GPIO HIGH = LED ON)
 */

#include <stdint.h>
#include <stdio.h>

/* ============================================================================
 * UART2 Function Declarations (from uart2_driver.c)
 * ============================================================================ */

void UART2_Init(void);
void UART2_SendChar(char c);
void UART2_SendString(const char *str);
void UART2_Printf(const char *format, ...);
char UART2_RecvChar(void);
uint8_t UART2_DataAvailable(void);

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
#define LED_DELAY_MS        500     /* 500ms - con người nhìn thấy được */
#define CLK_FREQ_MHZ        8       /* 8MHz HSI clock */

/* ============================================================================
 * TIMING UTILITIES
 * ============================================================================ */

/**
 * Delay in milliseconds (simple loop-based)
 * @ 8MHz, rough approximation
 */
static void delay_ms(uint32_t ms)
{
    while (ms--) {
        /* Delay ~1ms at 8MHz */
        uint32_t loops = 8000 / 3;  /* 8MHz / 3 cycles per loop ≈ 2.7kdel per ms */
        while (loops--) {
            __asm volatile("nop");
        }
    }
}

/**
 * Microsecond delay (rough approximation)
 * @ 8MHz, 1 iteration ≈ 3 CPU cycles
 * So: delay_us(500) ≈ 500µs (NOT RECOMMENDED FOR ACCURATE TIMING)
 */
static void delay_us(uint32_t us)
{
    uint32_t loops = us * CLK_FREQ_MHZ / 3;
    while (loops--) {
        __asm volatile("nop");
    }
}

/* ============================================================================
 * GPIO INITIALIZATION
 * ============================================================================ */

void GPIO_Init(void)
{
    /* Enable GPIOB clock (APB2) */
    RCC_APB2ENR |= (1 << 3);  /* GPIOB clock enable */
    
    /* Configure PB0-PB7 as outputs
     * CRL controls PB0-PB7 (bits 0-31)
     * Each pin uses 4 bits
     * Set mode=01, CNF=00 (push-pull output, 10MHz)
     */
    GPIOB_CRL &= 0x00000000;  /* Clear PB0-PB7 */
    GPIOB_CRL |= 0x11111111;  /* Set PB0-PB7 all outputs (mode=01) */
    
    /* Turn all LEDs off initially */
    GPIOB_ODR = 0x00;
}

/* ============================================================================
 * LED CONTROL FUNCTIONS
 * ============================================================================ */

/**
 * Turn on LED by number (0-7)
 * L0 = LED0 = PB0, L1 = LED1 = PB1, ..., L7 = LED7 = PB7
 * Active HIGH: GPIO HIGH = LED ON
 */
void LED_On(uint8_t led_num)
{
    if (led_num < LED_COUNT) {
        GPIOB_ODR |= (1 << led_num);  /* Set PB[led_num] HIGH */
        char msg[32];
        snprintf(msg, sizeof(msg), "[LED] LED%d = ON\r\n", led_num);
        UART2_SendString(msg);
    }
}

/**
 * Turn off LED by number (0-7)
 */
void LED_Off(uint8_t led_num)
{
    if (led_num < LED_COUNT) {
        GPIOB_ODR &= ~(1 << led_num);  /* Clear PB[led_num] LOW */
        char msg[32];
        snprintf(msg, sizeof(msg), "[LED] LED%d = OFF\r\n", led_num);
        UART2_SendString(msg);
    }
}

/**
 * Turn all LEDs on
 */
void LED_All_On(void)
{
    GPIOB_ODR = 0xFF;  /* Set PB0-PB7 HIGH */
    UART2_SendString("[LED] All LEDs ON\r\n");
}

/**
 * Turn all LEDs off
 */
void LED_All_Off(void)
{
    GPIOB_ODR = 0x00;  /* Clear PB0-PB7 LOW */
    UART2_SendString("[LED] All LEDs OFF\r\n");
}

/* ============================================================================
 * RUNNING LIGHT EFFECT (CHASE PATTERN)
 * ============================================================================ */

/**
 * Running light: turn ON sequentially (L1->L8), then OFF sequentially (L1->L8)
 * Each LED stays for LED_DELAY_MS before moving to next
 */
void LED_Running_Light(void)
{
    uint8_t i;
    
    /* ========== PHASE 1: Sequential ON (Sáng dồn) ========== */
    for (i = 0; i < LED_COUNT; i++) {
        LED_On(i);
        delay_ms(LED_DELAY_MS);
    }
    
    /* ========== PHASE 2: Sequential OFF (Tắt dồn) ========== */
    for (i = 0; i < LED_COUNT; i++) {
        LED_Off(i);
        delay_ms(LED_DELAY_MS);
    }
}

/**
 * Alternate running light: turn ON all, then chase OFF
 * (Different visual effect)
 */
void LED_Running_Light_Alt(void)
{
    uint8_t i;
    
    /* Start with all ON */
    LED_All_On();
    delay_ms(LED_DELAY_MS);
    
    /* Turn OFF sequentially */
    for (i = 0; i < LED_COUNT; i++) {
        LED_Off(i);
        delay_ms(LED_DELAY_MS);
    }
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void)
{
    /* Initialize GPIO for LEDs */
    GPIO_Init();

    /* Initialize UART2 for communication with F4 */
    UART2_Init();

    /* Startup message */
    UART2_SendString("\r\n");
    UART2_SendString("===========================================\r\n");
    UART2_SendString("[BOOT] STM32F103C8T6 LED Test Started\r\n");
    UART2_SendString("[INFO] UART2 @ 9600 baud (PA2=TX, PA3=RX)\r\n");
    UART2_SendString("[INFO] LEDs on PB0-PB7 (500ms delay)\r\n");
    UART2_SendString("===========================================\r\n");

    /* Loop forever: running light effect */
    uint32_t cycle = 0;
    while (1) {
        cycle++;
        char cycle_msg[64];
        snprintf(cycle_msg, sizeof(cycle_msg), "[CYCLE] Running light #%lu\r\n", cycle);
        UART2_SendString(cycle_msg);
        LED_Running_Light();
    }

    return 0;
}

/* ============================================================================
 * EXCEPTION HANDLERS (Weak stubs - NOT used, in boot file)
 * ============================================================================ */

void _exit(int status)
{
    (void)status;
    while (1);
}

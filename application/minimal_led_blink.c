/**
 * @file minimal_led_blink.c
 * @brief MINIMAL LED blink - No crashes, just pure GPIO toggle
 * @date 2026-03-24
 * 
 * PIN ASSIGNMENT:
 *   LED1 = PE13 (GPIO output, push-pull)
 *   LED2 = PE14 (GPIO output, push-pull)  
 *   LED3 = PE15 (GPIO output, push-pull)
 * 
 * OPERATION:
 *   1. Toggle all 3 LEDs together
 *   2. Delay using simple loop (no SysTick needed)
 *   3. Repeat infinitely
 */

#include <stdint.h>

/* ============================================================================
 * STM32F4 REGISTER DEFINITIONS
 * ============================================================================ */

#define RCC_BASE                0x40023800UL
#define RCC_AHB1ENR             (*(volatile uint32_t *)(RCC_BASE + 0x30))

#define GPIOE_BASE              0x40021000UL
#define GPIOE_MODER             (*(volatile uint32_t *)(GPIOE_BASE + 0x00))
#define GPIOE_OTYPER            (*(volatile uint32_t *)(GPIOE_BASE + 0x04))
#define GPIOE_OSPEEDR           (*(volatile uint32_t *)(GPIOE_BASE + 0x08))
#define GPIOE_PUPDR             (*(volatile uint32_t *)(GPIOE_BASE + 0x0C))
#define GPIOE_ODR               (*(volatile uint32_t *)(GPIOE_BASE + 0x14))
#define GPIOE_BSRR              (*(volatile uint32_t *)(GPIOE_BASE + 0x18))

/* ============================================================================
 * LED PIN DEFINITIONS
 * ============================================================================ */

#define LED1_PIN        13
#define LED2_PIN        14
#define LED3_PIN        15

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/**
 * @brief Simple delay using busy-wait loop
 * Calibrated for HSI 16MHz (SystemInit NOT called — reset-default clock).
 * Loop body (nop + subs + bne) ≈ 3 cycles at 16MHz → ~187ns per iteration.
 * For 1µs: 16 iterations is close enough (3µs overhead acceptable at this scale).
 * If PLL is later enabled (168MHz), change constant to 168.
 */
static inline void delay_us(uint32_t us)
{
    for (uint32_t i = 0; i < (us * 16); i++) {
        __asm volatile("nop");
    }
}

static inline void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        delay_us(1000);
    }
}

/**
 * @brief Initialize GPIO for LED output
 */
static void GPIO_Init(void)
{
    /* Enable GPIOE clock */
    RCC_AHB1ENR |= (1 << 4);
    
    /* Configure PE13, PE14, PE15 as output
       MODER: 2 bits per pin
       00 = input, 01 = output, 10 = alternate, 11 = analog
    */
    /* Clear then set PE13 as output */
    GPIOE_MODER &= ~(3U << (LED1_PIN * 2));
    GPIOE_MODER |=  (1U << (LED1_PIN * 2));
    
    /* Clear then set PE14 as output */
    GPIOE_MODER &= ~(3U << (LED2_PIN * 2));
    GPIOE_MODER |=  (1U << (LED2_PIN * 2));
    
    /* Clear then set PE15 as output */
    GPIOE_MODER &= ~(3U << (LED3_PIN * 2));
    GPIOE_MODER |=  (1U << (LED3_PIN * 2));
    
    /* Configure as push-pull (no open-drain)
       OTYPER: 0 = pushpull, 1 = open-drain
    */
    GPIOE_OTYPER &= ~((1 << LED1_PIN) | (1 << LED2_PIN) | (1 << LED3_PIN));
    
    /* Set speed to high (11b = 100MHz)
       OSPEEDR: 2 bits per pin
    */
    GPIOE_OSPEEDR |=  ((3U << (LED1_PIN * 2)) | 
                       (3U << (LED2_PIN * 2)) | 
                       (3U << (LED3_PIN * 2)));
    
    /* No pull-up/pull-down
       PUPDR: 2 bits per pin, 00 = no pull
    */
    GPIOE_PUPDR &= ~((3U << (LED1_PIN * 2)) | 
                     (3U << (LED2_PIN * 2)) | 
                     (3U << (LED3_PIN * 2)));
}

/**
 * @brief Turn on all LEDs (active-low: drive GPIO LOW to sink current through LED)
 */
static inline void LED_On(void)
{
    /* Upper 16 bits of BSRR = reset (clear pin LOW) → LED cathode at 0V → LED ON */
    GPIOE_BSRR = ((1U << LED1_PIN) | (1U << LED2_PIN) | (1U << LED3_PIN)) << 16;
}

/**
 * @brief Turn off all LEDs (active-low: drive GPIO HIGH → no current → LED OFF)
 */
static inline void LED_Off(void)
{
    /* Lower 16 bits of BSRR = set (drive pin HIGH) → LED cathode at 3.3V → LED OFF */
    GPIOE_BSRR = (1U << LED1_PIN) | (1U << LED2_PIN) | (1U << LED3_PIN);
}

/* ============================================================================
 * MAIN PROGRAM
 * ============================================================================ */

int main(void)
{
    /* Initialize GPIO for LED control */
    GPIO_Init();
    
    /* Main loop: blink all 3 LEDs */
    while (1) {
        /* Turn ON all LEDs */
        LED_On();
        delay_ms(500);
        
        /* Turn OFF all LEDs */
        LED_Off();
        delay_ms(500);
    }
    
    return 0;
}

/* ============================================================================
 * EXCEPTION HANDLERS (REQUIRED BY C RUNTIME)
 * ============================================================================ */

void HardFault_Handler(void)
{
    /* Infinite loop on hard fault - LED will be off */
    while (1);
}

void NMI_Handler(void)
{
    while (1);
}

void MemManage_Handler(void)
{
    while (1);
}

void BusFault_Handler(void)
{
    while (1);
}

void UsageFault_Handler(void)
{
    while (1);
}

void SVC_Handler(void)
{
    while (1);
}

void DebugMon_Handler(void)
{
    while (1);
}

void PendSV_Handler(void)
{
    while (1);
}

void SysTick_Handler(void)
{
    /* Empty SysTick handler */
}

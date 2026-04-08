/**
 * @file minimal_led_test.c
 * @brief ULTRA SIMPLE: Direct register toggle - no SystemInit dependency
 * @date 2026-03-28
 */

#include <stdint.h>

#define RCC_BASE                0x40023800UL
#define RCC_AHB1ENR             (*(volatile uint32_t *)(RCC_BASE + 0x30))

#define GPIOE_BASE              0x40021000UL
#define GPIOE_MODER             (*(volatile uint32_t *)(GPIOE_BASE + 0x00))
#define GPIOE_OTYPER            (*(volatile uint32_t *)(GPIOE_BASE + 0x04))
#define GPIOE_OSPEEDR           (*(volatile uint32_t *)(GPIOE_BASE + 0x08))
#define GPIOE_PUPDR             (*(volatile uint32_t *)(GPIOE_BASE + 0x0C))
#define GPIOE_ODR               (*(volatile uint32_t *)(GPIOE_BASE + 0x14))

#define LED1_PIN                13

/**
 * Simple counter-based delay (no SystemInit needed)
 */
static void simple_delay(uint32_t loops)
{
    while (loops--) {
        __asm volatile("nop");
    }
}

int main(void)
{
    /* Enable GPIOE clock */
    RCC_AHB1ENR |= (1U << 4);
    
    /* Configure PE13 as output - bits 26-27 in MODER */
    GPIOE_MODER &= ~(3U << 26);
    GPIOE_MODER |=  (1U << 26);     /* 01 = output */
    
    /* Configure as push-pull output (not open-drain) */
    GPIOE_OTYPER &= ~(1U << 13);    /* 0 = push-pull */
    
    /* No pull-up/pull-down */
    GPIOE_PUPDR &= ~(3U << 26);     /* 00 = no pull */
    
    /* Set speed to fast - bits 26-27 in OSPEEDR */
    GPIOE_OSPEEDR |= (3U << 26);    /* 11 = 100MHz */
    
    /* Start LED in OFF state (HIGH) */
    GPIOE_ODR |= (1U << LED1_PIN);
    
    /* Main loop - toggle LED */
    while (1) {
        /* LED ON (LOW) */
        GPIOE_ODR &= ~(1U << LED1_PIN);  /* 0 = ON (active low) */
        simple_delay(1000000);
        
        /* LED OFF (HIGH) */
        GPIOE_ODR |= (1U << LED1_PIN);   /* 1 = OFF */
        simple_delay(1000000);
    }
    
    return 0;
}

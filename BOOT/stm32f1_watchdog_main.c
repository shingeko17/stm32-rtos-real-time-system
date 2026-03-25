/**
 * @file stm32f1_watchdog_main.c
 * @brief STM32F103 Backup MCU - Watchdog & Failover Controller
 * @date 2026-03-22
 *
 * Functionality:
 * - Listen for heartbeat from F4 via UART2 (PA2)
 * - Detect F4 crash (2+ missed heartbeats)
 * - Activate failover motor control
 * - Watch status LED (PC13)
 */

#include "stm32f10x.h"

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

#define HEARTBEAT_TIMEOUT_MS  200
#define HEARTBEAT_PULSE       0xAA

/* IWDG (Independent Watchdog) - Hardware Watchdog for F1 itself
 * Uses internal LSI ~40kHz oscillator, independent of system clock.
 * Once started, cannot be disabled — resets F1 if not kicked in time.
 *
 * Timeout = (4 * 2^PR_value / LSI_Hz) * RLR
 * Config: PR=4 (/64) -> 40000/64 = 625 Hz -> 1.6ms/tick
 *         RLR=625 -> 625 * 1.6ms = 1000ms timeout
 */
#define IWDG_BASE_ADDR      0x40003000UL
#define IWDG_KR             (*(volatile uint32_t *)(IWDG_BASE_ADDR + 0x00U))
#define IWDG_PR             (*(volatile uint32_t *)(IWDG_BASE_ADDR + 0x04U))
#define IWDG_RLR            (*(volatile uint32_t *)(IWDG_BASE_ADDR + 0x08U))
#define IWDG_SR             (*(volatile uint32_t *)(IWDG_BASE_ADDR + 0x0CU))

#define IWDG_KEY_FEED       0xAAAAUL   /* Reload counter (kick) */
#define IWDG_KEY_UNLOCK     0x5555UL   /* Enable write to PR/RLR */
#define IWDG_KEY_START      0xCCCCUL   /* Start IWDG */

#define IWDG_PR_DIV64       0x04U      /* Prescaler /64 */
#define IWDG_RELOAD_1S      625U       /* 1000ms / 1.6ms per tick */

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================ */

static volatile uint32_t g_heartbeat_timer = 0;
static volatile uint8_t g_f4_alive = 1;
static volatile uint32_t g_system_ticks = 0;

/* ============================================================================
 * SYSTEM TICK (1ms)
 * ============================================================================ */

void SysTick_Handler(void)
{
    g_system_ticks++;
    
    if (g_heartbeat_timer > 0) {
        g_heartbeat_timer--;
    } else {
        /* Timeout occurred */
        if (g_f4_alive) {
            g_f4_alive = 0;
            /* Activate failover */
        }
    }
}

/* ============================================================================
 * HARDWARE INITIALIZATION
 * ============================================================================ */

void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* Enable GPIOC and GPIOA clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA, ENABLE);
    
    /* Configure PC13 as output (status LED) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    GPIO_ResetBits(GPIOC, GPIO_Pin_13);  /* LED off */
    
    /* Configure PA8-PA11 as motor control outputs */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_ResetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11);
}

void UART2_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    /* Enable UART2 and GPIOA clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    /* Configure PA2 (RX) as input */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* Configure PA3 (TX) as floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* UART2 configuration */
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx;
    
    USART_Init(USART2, &USART_InitStructure);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);
    
    /* Enable UART2 interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void SysTick_Config_Init(void)
{
    if (SysTick_Config(SystemCoreClock / 1000) != 0) {
        while (1);
    }
}

/* ============================================================================
 * INDEPENDENT WATCHDOG (IWDG) - Second watchdog for F1 self-supervision
 * ============================================================================ */

/**
 * @brief Start the IWDG with a ~1 second timeout.
 *        Call once at startup. Cannot be stopped after this.
 */
void IWDG_Init(void)
{
    /* Wait until LSI oscillator is ready (started by RCC automatically) */
    /* LSI is always enabled on STM32F1 once IWDG is accessed */

    /* Unlock PR and RLR registers */
    IWDG_KR = IWDG_KEY_UNLOCK;

    /* Wait until PR register is free to write */
    while (IWDG_SR & 0x01U) {}

    /* Set prescaler /64: tick = 40000Hz / 64 = 625Hz -> 1.6ms per tick */
    IWDG_PR = IWDG_PR_DIV64;

    /* Wait until RLR register is free to write */
    while (IWDG_SR & 0x02U) {}

    /* Reload value: 625 ticks * 1.6ms = 1000ms timeout */
    IWDG_RLR = IWDG_RELOAD_1S;

    /* Feed once to load the new reload value */
    IWDG_KR = IWDG_KEY_FEED;

    /* Start IWDG — cannot be disabled after this point */
    IWDG_KR = IWDG_KEY_START;
}

/**
 * @brief Kick (feed) the IWDG to prevent F1 reset.
 *        Must be called at least once every 1000ms.
 */
void IWDG_Feed(void)
{
    IWDG_KR = IWDG_KEY_FEED;
}

/* ============================================================================
 * MOTOR CONTROL FUNCTIONS (FAILOVER)
 * ============================================================================ */

void Motor_Stop_Failover(void)
{
    GPIO_ResetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11);
}

void Motor_Coast_Failover(void)
{
    /* Set IN1=0, IN2=0 (coasting) */
    GPIO_ResetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_9);  /* Motor A coast */
    GPIO_ResetBits(GPIOA, GPIO_Pin_10 | GPIO_Pin_11);/* Motor B coast */
}

void Motor_Brake_Failover(void)
{
    /* Set all pins HIGH for dynamic braking */
    GPIO_SetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11);
}

/* ============================================================================
 * SYSTEM STATUS
 * ============================================================================ */

void Update_LED_Status(void)
{
    if (g_f4_alive) {
        GPIO_SetBits(GPIOC, GPIO_Pin_13);   /* LED on = F4 is alive */
    } else {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);  /* LED off = F4 is dead */
    }
}

/* ============================================================================
 * UART2 INTERRUPT HANDLER
 * ============================================================================ */

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        uint8_t data = USART_ReceiveData(USART2);
        
        if (data == HEARTBEAT_PULSE) {
            /* Heartbeat received from F4 */
            g_heartbeat_timer = HEARTBEAT_TIMEOUT_MS;
            
            /* If was dead, now alive again */
            if (!g_f4_alive) {
                g_f4_alive = 1;
                Motor_Coast_Failover();  /* Resume controlled motion */
            }
        }
        
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}

/* ============================================================================
 * MAIN APPLICATION
 * ============================================================================ */

int main(void)
{
    /* Initialize system */
    RCC_DeInit();
    RCC_HSEConfig(RCC_HSE_ON);
    
    while (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET);
    
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_PCLK2Config(RCC_HCLK_Div1);
    
    /* Initialize peripherals */
    GPIO_Init();
    UART2_Init();
    SysTick_Config_Init();

    /* Start hardware watchdog — F1 resets if main loop stalls >1s */
    IWDG_Init();

    /* Set initial heartbeat timeout */
    g_heartbeat_timer = HEARTBEAT_TIMEOUT_MS;
    g_f4_alive = 1;

    /* Main loop */
    while (1) {
        /* Kick IWDG — must happen at least once per 1000ms */
        IWDG_Feed();

        /* Update LED status every 500ms */
        if ((g_system_ticks % 500) == 0) {
            Update_LED_Status();
        }

        /* Check F4 status */
        if (!g_f4_alive) {
            /* F4 is dead - maintain motor in safe state */
            Motor_Coast_Failover();
        }
    }
    
    return 0;
}

/* ============================================================================
 * MINIMAL STM32F1 HELPERS
 * ============================================================================ */

void GPIO_SetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    GPIOx->BSRR = GPIO_Pin;
}

void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    GPIOx->BRR = GPIO_Pin;
}

void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct)
{
    /* Simple GPIO init implementation for F1 */
}

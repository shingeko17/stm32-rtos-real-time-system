/**
 * @file uart_debug_test_code.c
 * @brief Ready-to-use test code for logic analyzer debugging
 * @description
 *   Copy-paste these test functions into your STM32 project
 *   and run them while capturing with logic analyzer
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Forward declarations (from uart driver) */
extern void UART_SendByte(uint8_t byte);
extern void UART_SendString(const char *str);
extern void delay_ms(uint32_t ms);

/* ============================================================================
 * TEST 1: Single Byte Transmission (Baudrate verification)
 * ============================================================================
 * 
 * Purpose: Verify baudrate is correct by checking bit timing
 * 
 * Expected on Logic Analyzer @ 115200:
 *   - Each bit should be 8.68µs ± 2%
 *   - 1 complete byte = 87µs (10 bits × 8.68µs)
 *   - 200ms gap between repeats
 */

void test_single_byte_timing(void)
{
    /**
     * Send 'A' (0x41 = 0100 0001)
     * Repeat every 200ms to see clear gaps
     * 
     * On logic analyzer:
     *   Zoom into first byte
     *   Measure bit width: should be exactly 8.68µs
     */
    printf("[TEST] Single Byte Timing - Sending 'A' repeatedly\r\n");
    printf("[INFO] Each byte should be ~87µs on wire\r\n");
    printf("[INFO] Gap between sends: 200ms\r\n");
    
    uint32_t count = 0;
    while (1) {
        UART_SendByte(0x41);  // 'A'
        count++;
        
        if (count % 5 == 0) {
            printf("[TEST] Sent %lu bytes\r\n", count);
        }
        
        delay_ms(200);  /* 200ms gap for visibility */
    }
}

/* ============================================================================
 * TEST 2: Complete String Transmission (Data integrity check)
 * ============================================================================
 * 
 * Purpose: Check if all bytes are transmitted and received
 * 
 * Expected on Logic Analyzer:
 *   TX: Shows clean "Hello World" bytes
 *   RX: Should mirror TX exactly
 * 
 * If RX missing bytes while TX OK → Blocking/ISR issue confirmed!
 */

void test_string_transmission(void)
{
    /**
     * Send "Hello World\r\n" repeatedly
     * 13 bytes total = ~1.13ms on wire
     * 500ms gap between repeats
     */
    printf("[TEST] String Transmission - Sending 'Hello World'\r\n");
    printf("[INFO] 13 bytes = ~1.13ms on wire\r\n");
    printf("[INFO] Gap between sends: 500ms\r\n");
    
    uint32_t count = 0;
    while (1) {
        UART_SendString("Hello World\r\n");
        count++;
        
        printf("[TEST] Cycle %lu complete\r\n", count);
        delay_ms(500);
    }
}

/* ============================================================================
 * TEST 3: Rapid Fire Test (ISR miss detection)
 * ============================================================================
 * 
 * Purpose: Send multiple bytes in quick succession
 * Check if RX ISR can keep up with TX
 * 
 * Expected on Logic Analyzer:
 *   TX: Rapid clean bytes (no gaps)
 *   RX: Should match TX
 *   If RX has gaps → ISR miss → Blocking confirmed!
 */

void test_rapid_transmission(void)
{
    /**
     * Send "ABCDEFGH" as fast as possible
     * Then wait 500ms for visibility
     */
    printf("[TEST] Rapid Fire - Sending 'ABCDEFGH' fast\r\n");
    printf("[INFO] Watch for RX gaps on logic analyzer\r\n");
    printf("[INFO] If RX != TX: ISR missing data (Blocking issue)\r\n");
    
    uint32_t cycle = 0;
    while (1) {
        /* Without delay - send as fast as possible */
        UART_SendString("ABCDEFGH");
        
        cycle++;
        if (cycle % 10 == 0) {
            printf("[TEST] Cycle %lu\r\n", cycle);
        }
        
        delay_ms(100);  /* Small gap to see pattern repeat */
    }
}

/* ============================================================================
 * TEST 4: Loopback Test (RX path verification)
 * ============================================================================
 * 
 * Prerequisite: Connect PA9 (TX) to PA10 (RX) on board
 * 
 * Purpose: Verify RX side by looping TX back to RX
 * 
 * Expected:
 *   TX: Clean "TEST123" bytes
 *   RX: Should see SAME bytes (because looped back)
 *   If RX ≠ TX → RX ISR issue
 */

void test_loopback_rx(void)
{
    /**
     * Standard loopback test
     * Requires: PA9 (TX) physically connected to PA10 (RX)
     */
    printf("[TEST] Loopback RX - Verifying RX path\r\n");
    printf("[INFO] Requires: PA9 (TX) connected to PA10 (RX)\r\n");
    printf("[INFO] TX and RX lines should be identical on analyzer\r\n");
    
    uint32_t count = 0;
    while (1) {
        UART_SendString("TEST123\r\n");
        count++;
        
        if (count % 5 == 0) {
            printf("[TEST] Loopback cycle %lu\r\n", count);
        }
        
        delay_ms(200);
    }
}

/* ============================================================================
 * TEST 5: Stress Test (Find breaking point)
 * ============================================================================
 * 
 * Purpose: Send continuously to find where ISR starts missing data
 * 
 * Expected:
 *   - At low speeds: RX = TX (OK)
 *   - At high speeds: RX might have gaps (ISR can't keep up)
 * 
 * If gaps appear with slow sends → Blocking issue confirmed
 * If gaps appear with fast sends → ISR too slow (might be normal at extreme rates)
 */

void test_continuous_stress(void)
{
    /**
     * Send continuously without delays
     * ISR will either keep up or drop data
     */
    printf("[TEST] Continuous Stress - Sending continuously\r\n");
    printf("[INFO] Watch RX line for gaps\r\n");
    printf("[INFO] Gaps = ISR dropping data = Blocking issue\r\n");
    
    uint32_t byte_count = 0;
    const char *pattern = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    int pattern_len = strlen(pattern);
    
    while (1) {
        for (int i = 0; i < pattern_len; i++) {
            UART_SendByte((uint8_t)pattern[i]);
            byte_count++;
        }
        
        UART_SendString("\r\n");
        byte_count += 2;
        
        if (byte_count % 500 == 0) {
            printf("[TEST] Sent %lu bytes\r\n", byte_count);
        }
    }
}

/* ============================================================================
 * TEST 6: Timed Bloat Test (With intentional delays to show blocking)
 * ============================================================================
 * 
 * Purpose: Deliberately show what happens when app code blocks
 * 
 * Expected behavior:
 *   1. Send byte
 *   2. Calculated 5ms delay (to show it's blocking)
 *   3. Send next byte
 *   
 *   On logic analyzer:
 *     TX shows: byte, 5ms gap, byte, 5ms gap, ...
 *     RX shows: same (because your own send is blocking)
 */

void test_intentional_blocking(void)
{
    /**
     * Simulate app code with delays
     * This shows EXACTLY what blocking looks like
     */
    printf("[TEST] Intentional Blocking - Demonstrating blocking effect\r\n");
    printf("[INFO] With current blocking UART_SendString()\r\n");
    printf("[INFO] You should see gaps on TX line\r\n");
    printf("[INFO] These gaps are the blocking delays!\r\n");
    
    uint32_t count = 0;
    while (1) {
        /* Send 1 byte */
        UART_SendString("H");
        
        /* Now blocking happens here for ~46µs */
        /* Then more delays for other bytes */
        
        /* Send full string - this will show blocking gaps if ISR starves */
        UART_SendString("ello\r\n");
        
        count++;
        printf("[TEST] Message %lu sent\r\n", count);
        
        delay_ms(200);  /* Gap for visibility */
    }
}

/* ============================================================================
 * TEST 7: Known Good Pattern (Baudrate reference)
 * ============================================================================
 * 
 * Purpose: Known pattern for manual verification
 * 
 * Send: 0x55, 0xAA, 0x55, 0xAA (alternating 0101..., 1010...)
 * This creates clear visual pattern to verify bit timing
 */

void test_known_pattern(void)
{
    /**
     * Send known test patterns:
     * 0x55 = 0101 0101 (alternating 0 and 1)
     * 0xAA = 1010 1010 (alternating 1 and 0)
     * 
     * These create clear visual patterns on oscilloscope
     */
    printf("[TEST] Known Pattern - Sending 0x55, 0xAA pattern\r\n");
    printf("[INFO] 0x55 = 01010101 (alternating low-high)\r\n");
    printf("[INFO] 0xAA = 10101010 (alternating high-low)\r\n");
    printf("[INFO] Easy to verify bit timing visually\r\n");
    
    uint32_t cycle = 0;
    while (1) {
        UART_SendByte(0x55);  /* 0101 0101 */
        
        cycle++;
        if (cycle % 10 == 0) {
            printf("[TEST] Pattern cycle %lu\r\n", cycle);
        }
        
        delay_ms(100);
        
        UART_SendByte(0xAA);  /* 1010 1010 */
        delay_ms(100);
    }
}

/* ============================================================================
 * MAIN TEST SELECTOR
 * ============================================================================
 */

/**
 * Call one of these from your main() to run the test
 * 
 * Example:
 *   int main(void) {
 *       UART_Init();
 *       test_single_byte_timing();  // Or any other test
 *       return 0;
 *   }
 */

int run_uart_debug_tests(int test_number)
{
    printf("\r\n====== UART DEBUG TEST ======\r\n");
    printf("Starting test %d\r\n", test_number);
    printf("Start logic analyzer capture NOW!\r\n");
    printf("=============================\r\n\r\n");
    
    switch (test_number) {
        case 1:
            test_single_byte_timing();
            break;
        case 2:
            test_string_transmission();
            break;
        case 3:
            test_rapid_transmission();
            break;
        case 4:
            test_loopback_rx();
            break;
        case 5:
            test_continuous_stress();
            break;
        case 6:
            test_intentional_blocking();
            break;
        case 7:
            test_known_pattern();
            break;
        default:
            printf("[ERROR] Unknown test number: %d\r\n", test_number);
            printf("[INFO] Valid tests: 1-7\r\n");
            return -1;
    }
    
    return 0;
}

/* ============================================================================
 * QUICK START EXAMPLE (Copy into your main.c)
 * ============================================================================
 */

/*
// In your main.c:

int main(void) {
    UART_Init();
    
    // Start test 1: Single byte timing
    run_uart_debug_tests(1);
    
    // Start logic analyzer AFTER this point
    // Capture 1-2 seconds
    
    return 0;
}
*/

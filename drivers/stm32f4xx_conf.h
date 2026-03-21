/**
 * @file stm32f4xx_conf.h
 * @brief STM32F4xx Standard Peripheral Library - Enabled Drivers Configuration
 * @details File này include chỉ những driver thực tế cần dùng
 *          Giảm thời gian compile bằng cách không include những driver không cần
 * @author STM32 Configuration Team
 * @date 2026-03-14
 * 
 * ============================================================================
 * HƯỚNG DẪN: Uncomment chỉ những #include nào project cần dùng
 * ============================================================================
 */

#ifndef STM32F4XX_CONF_H
#define STM32F4XX_CONF_H

/* ============================================================================
 * INCLUDE CÁC DRIVER CẦN THIẾT CHO PROJECT LM298N MOTOR
 * ============================================================================ */

/* --- Driver RCC (Reset and Clock Control) - BẮT BUỘC ---
   Mục đích: Cấu hình clock, bật/tắt clock cho các peripheral
   Cần dùng: Có (để bật clock cho GPIO, TIM)
   Include: #include "stm32f4xx_rcc.h"
*/
#include "stm32f4xx_rcc.h"

/* --- Driver GPIO (General Purpose I/O) - BẮT BUỘC ---
   Mục đích: Khởi tạo pin, set/reset pin output
   Cần dùng: Có (để điều khiển IN1-IN4 pins)
   Include: #include "stm32f4xx_gpio.h"
*/
#include "stm32f4xx_gpio.h"

/* --- Driver TIM (Timer) - BẮT BUỘC ---
   Mục đích: Cấu hình timer, PWM output
   Cần dùng: Có (để generate PWM cho ENA, ENB)
   Include: #include "stm32f4xx_tim.h"
*/
#include "stm32f4xx_tim.h"

/* --- Driver MISC (Miscellaneous) - BẮT BUỘC ---
   Mục đích: NVIC (Nested Vectored Interrupt Controller), SysTick
   Cần dùng: Có (nếu dùng interrupt hoặc Delay)
   Include: #include "misc.h"
*/
#include "misc.h"

/* ============================================================================
 * CÁC DRIVER KHÔNG DÙNG - GHI CHÚ LẠI CHỈ
 * ============================================================================ */

/* === Driver ADC (Analog to Digital Converter) ===
   Mục đích: Đọc cảm biến analog (nhiệt độ, điện áp, ...)
   Cần dùng: Không (project này chỉ điều khiển motor)
   Include: // #include "stm32f4xx_adc.h"
*/
/* #include "stm32f4xx_adc.h" */

/* === Driver USART (Serial Port) ===
   Mục đích: Giao tiếp RS232, RS485, debug serial
   Cần dùng: Không (nhưng có thể thêm sau để debug)
   Include: // #include "stm32f4xx_usart.h"
*/
/* #include "stm32f4xx_usart.h" */

/* === Driver SPI (Serial Peripheral Interface) ===
   Mục đích: Giao tiếp SPI (SD card, EEPROM, NRF24, ...)
   Cần dùng: Không (không dùng thiết bị SPI)
   Include: // #include "stm32f4xx_spi.h"
*/
/* #include "stm32f4xx_spi.h" */

/* === Driver I2C (Inter-Integrated Circuit) ===
   Mục đích: Giao tiếp I2C (sensor, EEPROM, LCD, ...)
   Cần dùng: Không (không dùng thiết bị I2C)
   Include: // #include "stm32f4xx_i2c.h"
*/
/* #include "stm32f4xx_i2c.h" */

/* === Driver EXTI (External Interrupt) ===
   Mục đích: Xử lý interrupt từ button/sensor ngoài
   Cần dùng: Không (project này không dùng external interrupt)
   Include: // #include "stm32f4xx_exti.h"
*/
/* #include "stm32f4xx_exti.h" */

/* === Driver SYSCFG (System Configuration) ===
   Mục đích: Cấu hình system (remap pin, event output, ...)
   Cần dùng: Không (không cần remap pin)
   Include: // #include "stm32f4xx_syscfg.h"
*/
/* #include "stm32f4xx_syscfg.h" */

/* === Driver DMA (Direct Memory Access) ===
   Mục đích: Transfer dữ liệu mà không cần CPU (UART, ADC, SPI, ...)
   Cần dùng: Không (transfer dữ liệu ít)
   Include: // #include "stm32f4xx_dma.h"
*/
/* #include "stm32f4xx_dma.h" */

/* === Driver DAC (Digital to Analog Converter) ===
   Mục đích: Output audio hoặc tín hiệu analog
   Cần dùng: Không (không cần analog output)
   Include: // #include "stm32f4xx_dac.h"
*/
/* #include "stm32f4xx_dac.h" */

/* === Driver CRC (Cyclic Redundancy Check) ===
   Mục đích: Kiểm tra lỗi checksum
   Cần dùng: Không (không cần CRC)
   Include: // #include "stm32f4xx_crc.h"
*/
/* #include "stm32f4xx_crc.h" */

/* === Driver RTC (Real Time Clock) ===
   Mục đích: Đồng hồ thời gian thực
   Cần dùng: Không (không cần RTC)
   Include: // #include "stm32f4xx_rtc.h"
*/
/* #include "stm32f4xx_rtc.h" */

/* === Driver IWDG (Independent Watchdog) ===
   Mục đích: Watchdog timer (tự động reset nếu firmware bị hang)
   Cần dùng: Không (nhưng nên thêm sau cho production)
   Include: // #include "stm32f4xx_iwdg.h"
*/
/* #include "stm32f4xx_iwdg.h" */

/* === Driver WWDG (Window Watchdog) ===
   Mục đích: Window watchdog timer
   Cần dùng: Không
   Include: // #include "stm32f4xx_wwdg.h"
*/
/* #include "stm32f4xx_wwdg.h" */

/* === Driver PWR (Power Management) ===
   Mục đích: Quản lý chế độ sleep, power saving
   Cần dùng: Không (project này không dùng sleep mode)
   Include: // #include "stm32f4xx_pwr.h"
*/
/* #include "stm32f4xx_pwr.h" */

/* === Driver CAN (Controller Area Network) ===
   Mục đích: Giao tiếp CAN bus (automotive, robot)
   Cần dùng: Không (không dùng CAN)
   Include: // #include "stm32f4xx_can.h"
*/
/* #include "stm32f4xx_can.h" */

/* === Driver FLASH (Flash Memory) ===
   Mục đích: Erase/write flash memory (firmware update, EEPROM simulation)
   Cần dùng: Không (chỉ read flash, không write)
   Include: // #include "stm32f4xx_flash.h"
*/
/* #include "stm32f4xx_flash.h" */

/* === Driver DCMI (Digital Camera Interface) ===
   Mục đích: Camera input
   Cần dùng: Không (không có camera)
   Include: // #include "stm32f4xx_dcmi.h"
*/
/* #include "stm32f4xx_dcmi.h" */

/* === Driver CRYP (Cryptography) ===
   Mục đích: Mã hóa/giải mã
   Cần dùng: Không
   Include: // #include "stm32f4xx_cryp.h"
*/
/* #include "stm32f4xx_cryp.h" */

/* === Driver HASH (Hash Engine) ===
   Mục đích: Tính hash (SHA-1, MD5)
   Cần dùng: Không
   Include: // #include "stm32f4xx_hash.h"
*/
/* #include "stm32f4xx_hash.h" */

/* === Driver RNG (Random Number Generator) ===
   Mục đích: Sinh số ngẫu nhiên
   Cần dùng: Không
   Include: // #include "stm32f4xx_rng.h"
*/
/* #include "stm32f4xx_rng.h" */

/* ============================================================================
 * KẾT THÚC STM32F4XX_CONF_H
 * ============================================================================ */

#endif  /* STM32F4XX_CONF_H */

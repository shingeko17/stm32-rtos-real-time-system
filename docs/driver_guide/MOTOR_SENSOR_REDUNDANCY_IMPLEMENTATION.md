# Motor + Sensor + Redundancy Implementation (WIP)

## Scope switched from LED to motor stack

This implementation wave moves runtime from LED blink to:
- Motor control on STM32F4
- Sensor path priority: Encoder RPM + ADC current/temperature
- Heartbeat transport F4 -> F1 for redundancy supervision

## What is implemented now

### F4 runtime path
- Application entry changed to motor-sensor control loop:
  - application/motor_sensor_main.c
- Active modules in build:
  - drivers/BSP/bsp_motor.c
  - drivers/api/motor_driver.c
  - drivers/api/encoder_driver.c
  - drivers/api/adc_driver.c
  - drivers/api/uart_driver.c

### Driver status
- Encoder:
  - TIM3 input capture on PA6
  - IRQ pulse counting in TIM3_IRQHandler
  - RPM derived from pulse delta per ENCODER_SAMPLE_RATE_MS
- ADC + DMA:
  - ADC1 continuous scan channels 0/1/2
  - DMA2 Stream0 circular transfer to local buffer
  - Read APIs expose averaged channel value
- UART heartbeat:
  - USART1 register-level init
  - blocking transmit APIs
  - heartbeat byte sending

### Build split
- F4 wrapper:
  - Makefile.f4
- F1 separate makefile skeleton:
  - Makefile.f1
- F1 linker script scaffold:
  - BOOT/stm.ld.f1

## Remaining blockers

1. F1 vendor package is not in repo yet (stm32f10x.h and related library), so F1 build is scaffold-only currently.
2. F4 closed-loop control is still open-loop mapping (RPM feedback read is available; PID not integrated yet).
3. No integration harness yet for forced-heartbeat-loss takeover timing measurement.

## Next implementation slice

1. Add F1 startup file and vendor headers, then make -f Makefile.f1 pass.
2. Complete F1 full-takeover state machine in application/application_f1_watchdog_main.c.
3. Add closed-loop speed control step on F4 (PID or PI) using Encoder_GetRPM().
4. Add failure-injection mode to stop heartbeat and verify F1 takeover latency.

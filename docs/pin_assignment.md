# Pin Assignment - STM32F407VET6 + STM32F103C8

## F4 (STM32F407VET6) - Primary MCU

### Motor Control (PWM + Encoder)

| Function | Pin | Timer | AF | Purpose |
|----------|-----|-------|----|---------| 
| PWM_A | PA8 | TIM1_CH1 | AF1 | Motor A PWM (0-100%) |
| PWM_B | PA9 | TIM1_CH2 | AF1 | Motor B PWM (0-100%) |
| DIR_A1 | PD0 | GPIO | - | Motor A direction bit 1 |
| DIR_A2 | PD1 | GPIO | - | Motor A direction bit 2 |
| DIR_B1 | PD2 | GPIO | - | Motor B direction bit 1 |
| DIR_B2 | PD3 | GPIO | - | Motor B direction bit 2 |
| ENCODER_A | PA6 | TIM3_CH1 | AF2 | Motor encoder feedback A |
| ENCODER_B | PA7 | TIM3_CH2 | AF2 | Motor encoder feedback B |

**Motor Configuration:**
- Frequency: 1kHz (TIM1 configured at 1kHz)
- Resolution: 8-bit (0-255 duty cycle)
- Direction: 2-bit control (00=stop, 01=fwd, 10=rev, 11=brake)
- Encoder: Quadrature (TIM3 counter mode)

---

### Sensor Inputs (ADC + DMA)

| Sensor | Pin | ADC Channel | Purpose |
|--------|-----|-------------|---------|
| Current_A | PA0 | ADC1_IN0 | Motor A current sense |
| Current_B | PA1 | ADC1_IN1 | Motor B current sense |
| Temp | PA2 | ADC1_IN2 | Temperature sensor (NTC/LM35) |
| Speed_ref | PA3 | ADC1_IN3 | Speed reference pot (optional) |

**DMA Configuration:**
- ADC1 → DMA2_Stream4 (circular mode, ring buffer)
- Sample rate: 10kHz (ADC frequency 36MHz, prescaler 4)
- Conversion time: ~10µs per channel

---

### UART Communications

| Function | TX | RX | UART | Baud |
|----------|----|----|------|------|
| **Heartbeat to F1** | PC6 | - | UART6 | 115200 |
| **Telemetry to Host** | PA9 | PA10 | UART1 | 115200 |

**Heartbeat Protocol (F4 → F1):**
- Message: Single byte `0xAA` (170 decimal)
- Period: 50-100ms (configurable via #define)
- Purpose: Keep-alive signal to STM32F1 watchdog

**Telemetry Protocol (F4 → Host PC):**
- Format: CSV or binary (TBD)
- Fields: Motor speed, current, temperature, CPU load %
- Buffer: 128 bytes (UART FIFO)

---

## F1 (STM32F103C8) - Backup MCU

### Watchdog & Communication

| Function | Pin | UART | Purpose |
|----------|-----|------|---------|
| **Listen Heartbeat** | PA2 | UART2 RX | Receive 0xAA from F4 |
| **Watchdog Output** | PC13 | GPIO | LED/status indicator |
| **Failover Motor PWM** | PA8 | TIM1_CH1 | Backup PWM (if F4 fails) |
| **Failover Motor Dir** | PB0-PB3 | GPIO | Backup motor direction |

**Watchdog Timing:**
- Heartbeat expected every: 50-100ms
- Timeout threshold: 200ms (2× period)
- Action on timeout: Activate motor control with safe default (coast)
- Recovery: If heartbeat resumes, deactivate failover

---

## Unused Pins (Removed from old architecture)

**NOT USED (original IoT Gateway pins):**
- ❌ Ethernet (ETH_RMII_*) - Not in scope
- ❌ CAN Bus (CAN1/CAN2) - Not in scope
- ❌ RS485 (USART2 as Modbus) - Not in scope
- ❌ NRF24L01 (SPI) - Not in scope
- ❌ SD Card (SDIO) - Not in scope

---

## Pin Summary

| MCU | Used Pins | Available Pins | Total |
|-----|-----------|----------------|-------|
| F4 | 16 (motor, ADC, UART) | 84 | 100 (LQFP100) |
| F1 | 8 (watchdog, motor backup) | 40 | 48 (LQFP48) |

**Notes:**
1. All timers configured for 168MHz / 72MHz clocks respectively
2. DMA channels don't conflict (motor PWM ≠ ADC DMA)
3. UART6 & UART1 on separate peripherals (no conflict)
4. F1 operates independently (minimal dependencies on F4)
| ETH_RMII_TXD1 | PB13 | Transmit data bit 1 | |

**PHY Chip**: DP83848CVV/NOPB (LQFP48)  
**PHY Address**: 0x01 (verify from schematic - may be 0x00)  
**Reference Clock**: Provided by PHY to MCU

---

## SPI - External Flash (W25Q64)

| Signal | Pin | Function | Notes |
|--------|-----|----------|-------|
| SPI_SCK | **(TBD)** | Clock | Max 104MHz (typ. use 42MHz) |
| SPI_MISO | **(TBD)** | Master In Slave Out | |
| SPI_MOSI | **(TBD)** | Master Out Slave In | |
| SPI_CS | **(TBD)** | Chip Select (active low) | GPIO control |

**Flash Chip**: W25Q64JVSSIQ (8MB = 64Mbit)  
**SPI Mode**: Mode 0 or Mode 3  
**Max SPI Clock**: 104MHz (check datasheet for your specific chip)

**Typical SPI Peripheral**: SPI1 or SPI2

---

## I2C - EEPROM (24C02)

| Signal | Pin | Function | Notes |
|--------|-----|----------|-------|
| I2C_SCL | **(TBD)** | Clock | Max 400kHz (Fast mode) |
| I2C_SDA | **(TBD)** | Data | Open-drain with pull-up |

**EEPROM Chip**: 24C02 (256 bytes, 16 pages × 16 bytes)  
**I2C Address**: 0x50 (7-bit), may vary if A0/A1/A2 pins configured  
**Page Write**: 16 bytes max  
**Write Cycle Time**: 5ms typical

**Typical I2C Peripheral**: I2C1 or I2C2

---

## SD Card (TF Card)

| Signal | Pin | SPI Mode | SDIO Mode | Notes |
|--------|-----|----------|-----------|-------|
| CLK | **(TBD)** | SPI SCK | SDIO_CK | |
| CMD | **(TBD)** | SPI MOSI | SDIO_CMD | |
| D0 | **(TBD)** | SPI MISO | SDIO_D0 | |
| D1 | **(TBD)** | - | SDIO_D1 | SDIO only |
| D2 | **(TBD)** | - | SDIO_D2 | SDIO only |
| D3 | **(TBD)** | SPI CS | SDIO_D3 | |
| CD | **(TBD)** | Card detect (optional) | GPIO | |

**Modes**:
- **SPI**: Simpler, slower (max ~10-20 Mbps)
- **SDIO 4-bit**: Faster (max ~24 Mbps @ 48MHz clock)

**Recommendation**: Start with SPI mode, migrate to SDIO if performance needed

**Init Sequence**:
1. SPI clock ≤ 400kHz during init
2. Send CMD0 (reset)
3. After init complete, increase to max speed (e.g., 18MHz)

---

## Motor Control (L298N H-Bridge)

| Signal | Pin | Function | Timer/GPIO | Notes |
|--------|-----|----------|------------|-------|
| IN1 | **(TBD)** | Motor A direction | GPIO Output | |
| IN2 | **(TBD)** | Motor A direction | GPIO Output | |
| ENA | **(TBD)** | Motor A speed (PWM) | TIM1_CH1 | 20kHz PWM recommended |
| IN3 | **(TBD)** | Motor B direction | GPIO Output | |
| IN4 | **(TBD)** | Motor B direction | GPIO Output | |
| ENB | **(TBD)** | Motor B speed (PWM) | TIM3_CH1 | 20kHz PWM recommended |

**L298N Control Logic:**
```
Motor A:
  IN1=1, IN2=0, ENA=PWM → Forward
  IN1=0, IN2=1, ENA=PWM → Reverse
  IN1=0, IN2=0, ENA=X   → Brake (short brake)
  IN1=1, IN2=1, ENA=X   → Brake (short brake)
  
Motor B: Same logic with IN3/IN4/ENB
```

**PWM Configuration:**
- Frequency: 20kHz (above audible range, smooth operation)
- Duty cycle: 0-100% (0% = stop, 100% = full speed)

**Optional - Encoder Feedback:**
- Motor A Encoder: TIM2_CH1 (PA0), TIM2_CH2 (PA1) - Quadrature mode
- Motor B Encoder: TIM4_CH1 (PB6), TIM4_CH2 (PB7) - Quadrature mode

---

## NRF24L01 (2.4GHz Wireless)

| Signal | Pin | Function | Notes |
|--------|-----|----------|-------|
| SPI_SCK | **(TBD)** | Clock | Shared SPI or dedicated |
| SPI_MISO | **(TBD)** | MISO | |
| SPI_MOSI | **(TBD)** | MOSI | |
| CSN | **(TBD)** | Chip select | GPIO control |
| CE | **(TBD)** | Chip enable | GPIO control |
| IRQ | **(TBD)** | Interrupt (optional) | GPIO + EXTI |

**Connector**: P3 header (verify pinout from board silkscreen)  
**Max SPI Clock**: 10MHz  
**Supply Voltage**: 3.3V (1.9V - 3.6V range)

---

## DS18B20 Temperature Sensor (1-Wire)

| Signal | Pin | Function | Notes |
|--------|-----|----------|-------|
| DQ (Data) | **(TBD)** | Data line | GPIO with 4.7kΩ pull-up to 3.3V |

**Protocol**: 1-Wire (Dallas/Maxim)  
**Pull-up Resistor**: 4.7kΩ to VDD (required)  
**Resolution**: 9-12 bits configurable (default 12-bit = 0.0625°C)  
**Conversion Time**: ~750ms @ 12-bit resolution

---

## USB OTG Full-Speed

| Peripheral | DP Pin | DM Pin | ID Pin | VBUS Pin | Function |
|------------|--------|--------|--------|----------|----------|
| USB_OTG_FS | PA12 | PA11 | PA10 | PA9 | Host/Device |

**⚠️ POTENTIAL CONFLICT**: 
- If board uses PA9/PA10 for USART1 (RS232), USB may need different pins
- Verify from schematic which interface is actually wired

**USB Modes**:
- **Device**: Virtual COM Port, Mass Storage, HID
- **Host**: USB flash drives, keyboards, mice

---

## User Interface

| Function | Pin | Type | Active State | Notes |
|----------|-----|------|--------------|-------|
| LED1 | **(TBD)** | GPIO Output | High = ON | User LED 1 |
| LED2 | **(TBD)** | GPIO Output | High = ON | User LED 2 |
| LED3 | **(TBD)** | GPIO Output | High = ON | User LED 3 |
| K1 Button | **(TBD)** | GPIO Input | Low (pressed) | Internal pull-up |
| K2 Button | **(TBD)** | GPIO Input | Low (pressed) | Internal pull-up |
| K3 Button | **(TBD)** | GPIO Input | Low (pressed) | Internal pull-up |

**Debouncing**: Implement in software (10-50ms delay)

---

## Clock Sources

| Source | Frequency | Pins | Usage | Notes |
|--------|-----------|------|-------|-------|
| HSE (External Crystal) | 8MHz | PH0, PH1 | Main system clock | PLL → 168MHz |
| LSE (External Crystal) | 32.768kHz | PC14, PC15 | RTC | Optional, may not be populated |
| HSI (Internal RC) | 16MHz | - | Backup clock | Less accurate than HSE |

**PLL Configuration (168MHz)**:
```
HSE (8MHz) → PLL:
  PLLM = 8   (8MHz / 8 = 1MHz)
  PLLN = 336 (1MHz × 336 = 336MHz)
  PLLP = 2   (336MHz / 2 = 168MHz) ← SYSCLK
  PLLQ = 7   (336MHz / 7 = 48MHz)  ← USB, RNG, SDIO
```

---

## JTAG/SWD Debug Interface

| Signal | Pin | Function | Notes |
|--------|-----|----------|-------|
| SWDIO | PA13 | Serial Wire Debug I/O | Data line |
| SWCLK | PA14 | Serial Wire Clock | Clock line |
| SWO | PB3 | Serial Wire Output | Optional, for ITM trace |
| NRST | NRST | Reset | Active low |

**Debugger**: ST-Link V2, J-Link, or compatible  
**Interface**: SWD (Serial Wire Debug) - 2 wires only (SWDIO, SWCLK)

---

## Power Pins

| Pin | Voltage | Function | Notes |
|-----|---------|----------|-------|
| VDD (multiple) | 3.3V | Digital supply | Connect all VDD pins |
| VDDA | 3.3V | Analog supply | ADC reference |
| VBAT | 3.3V | Backup domain | RTC, backup registers |
| VSS (multiple) | 0V | Ground | Connect all VSS pins |
| VREF+ | 3.3V | ADC positive reference | Usually tied to VDDA |

**Decoupling Capacitors**:
- 100nF ceramic per VDD pin (close to pin)
- 10µF tantalum/electrolytic per power section
- 1µF ceramic on VDDA

---

## Pin Summary by Peripheral

| Peripheral | Pins Used | Notes |
|------------|-----------|-------|
| Ethernet (RMII) | PA1, PA2, PA7, PC1, PC4, PC5, PB11, PB12, PB13 | 9 pins |
| CAN1 | PD0, PD1 | 2 pins |
| CAN2 | PB5, PB6 | 2 pins |
| USART1 (RS232) | PA9, PA10 | 2 pins |
| USART2 (RS485) | PD5, PD6 | 2 pins + 1 GPIO (DE/RE) |
| USB | PA11, PA12, (PA9, PA10) | 2-4 pins (conflict check) |
| L298N Motor | 6 GPIO + 2 PWM | 8 pins |
| SPI Flash | 4 pins (TBD) | SPI + CS |
| I2C EEPROM | 2 pins (TBD) | SCL + SDA |
| SD Card | 4-7 pins (TBD) | SPI or SDIO mode |
| NRF24L01 | 6 pins (TBD) | SPI + CE + CSN + IRQ |
| DS18B20 | 1 pin (TBD) | 1-Wire data |
| LEDs | 3 pins (TBD) | GPIO outputs |
| Buttons | 3 pins (TBD) | GPIO inputs |
| Debug (SWD) | PA13, PA14, PB3 | SWDIO, SWCLK, SWO |

**Total Estimated**: ~55-60 pins used

---

## Action Items - TODO

- [ ] Verify SPI Flash (W25Q64) pin assignment from schematic
- [ ] Verify I2C EEPROM (24C02) pin assignment from schematic
- [ ] Verify SD Card interface pins and mode (SPI vs SDIO)
- [ ] Verify L298N motor control pins (IN1-4, ENA, ENB)
- [ ] Verify NRF24L01 P3 connector pinout
- [ ] Verify DS18B20 temperature sensor pin
- [ ] Verify LED pins (LED1, LED2, LED3)
- [ ] Verify button pins (K1, K2, K3)
- [ ] Verify USB pins (check conflict with USART1)
- [ ] Verify RS485 DE/RE control pin
- [ ] Check if LSE (32.768kHz) crystal is populated
- [ ] Measure/verify Ethernet RMII_REF_CLK source

**How to Verify**:
1. Request board schematic from manufacturer/seller
2. Use multimeter continuity mode to trace connections
3. Check silkscreen labels on PCB
4. Refer to DP83848 PHY datasheet for Ethernet pins
5. Test with simple blink/toggle code to confirm GPIO pins
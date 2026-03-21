# Peripheral Drivers Configuration - STM32F407VET6

**Purpose**: Configure and use STM32F407VET6 peripherals  
**HAL Library**: STM32F4xx_HAL  
**FreeRTOS Integration**: ISR nesting, task notifications

---

## Table of Contents

1. [UART/USART Drivers](#uart)
2. [CAN Bus Drivers](#can)
3. [Ethernet Driver](#ethernet)
4. [SPI Driver (External Flash)](#spi)
5. [I2C Driver (EEPROM)](#i2c)
6. [ADC Driver (Sensors)](#adc)
7. [PWM/Timer Drivers](#pwm)
8. [GPIO Driver](#gpio)
9. [DMA Configuration](#dma)

---

## <a id="uart"></a>UART/USART Drivers

### USART1 - RS232 Debug/PC Communication

**Configuration**:
- Baud Rate: 115200 bps
- Data Bits: 8
- Stop Bits: 1
- Parity: None
- Flow Control: None (CTS/RTS available)
- TX: PA9, RX: PA10
- Clock: APB2 (84 MHz after prescaler)

**HAL Initialization**:
```c
UART_HandleTypeDef huart1;

void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    
    HAL_UART_Init(&huart1);
    
    // Enable UART interrupts
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
}
```

**DMA Support**:
```c
// Enable DMA for TX (non-blocking)
HAL_UART_Transmit_DMA(&huart1, (uint8_t *)data, length);

// Enable DMA for RX (circular buffer)
HAL_UART_Receive_DMA(&huart1, (uint8_t *)rx_buffer, BUFFER_SIZE);
```

**IRQ Handler (Ring Buffer)**:
```c
RingBuffer_t uart1_rx_buffer;

void USART1_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {
        uint8_t ch = (uint8_t)READ_REG(huart1.Instance->DR);
        RingBuffer_Write(&uart1_rx_buffer, ch);
        __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE);
    }
}
```

### USART2 - RS485 Modbus RTU

**Configuration**:
- Baud Rate: 19200 bps (Industrial standard)
- Data Bits: 8
- Stop Bits: 1
- Parity: Even
- TX: PD5, RX: PD6
- DE/RE Control: GPIO pin (manual or automatic)

**HAL Initialization**:
```c
UART_HandleTypeDef huart2;

void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 19200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_EVEN;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    
    HAL_UART_Init(&huart2);
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
}
```

**RS485 Direction Control**:
```c
#define RS485_DE_PIN    GPIO_PIN_8    // DE on GPIOB
#define RS485_DE_PORT   GPIOB

void RS485_SetTransmit(uint8_t enable)
{
    if (enable) {
        HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_SET);
        HAL_Delay(1);  // Setup time
    } else {
        HAL_Delay(5);  // TX complete delay
        HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_RESET);
    }
}

void USART2_Transmit_Modbus(uint8_t *data, uint16_t len)
{
    RS485_SetTransmit(1);
    HAL_UART_Transmit(&huart2, data, len, 1000);
    RS485_SetTransmit(0);
}
```

---

## <a id="can"></a>CAN Bus Drivers

### CAN1 - Primary CAN Bus

**Configuration**:
- Baud Rate: 500 kbps
- Controller: CAN1 (PD0: RX, PD1: TX)
- Sample Point: 87.5%
- Transceiver: TJA1050 or SN65HVD230

**Timer Calculation** (for 500k baud @ 42MHz APB1):
- PCLK1 = 42 MHz
- Time Quantum = 2 × PCLK1 / (Prescaler × (SJW+BS1+BS2+1))
- For 500k: Prescaler=3, BS1=12, BS2=2, SJW=1

**HAL Initialization**:
```c
CAN_HandleTypeDef hcan1;

void MX_CAN1_Init(void)
{
    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = 3;
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_12TQ;
    hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = DISABLE;
    hcan1.Init.AutoWakeUp = DISABLE;
    hcan1.Init.AutoRetransmission = ENABLE;
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;
    
    HAL_CAN_Init(&hcan1);
    
    // Configure filter
    CAN_FilterTypeDef sFilterConfig = {0};
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    
    HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig);
    
    // Start CAN
    HAL_CAN_Start(&hcan1);
    
    // Enable RX interrupts
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
}
```

**CAN Message Transmission**:
```c
void CAN_Transmit_Message(uint32_t id, uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    
    TxHeader.StdId = id;
    TxHeader.ExtId = 0;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = len;
    TxHeader.TransmitGlobalTime = DISABLE;
    
    HAL_CAN_AddTxMessage(&hcan1, &TxHeader, data, &TxMailbox);
}
```

**CAN Message Reception**:
```c
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];
    
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData);
    
    // Process CAN message
    printf("CAN ID: 0x%03X, DLC: %d\n", RxHeader.StdId, RxHeader.DLC);
    
    // Send to queue/task
    xQueueSendFromISR(can_queue, &RxHeader, NULL);
}
```

---

## <a id="ethernet"></a>Ethernet Driver

### MAC Configuration (RMII Mode)

**Pins** (already documented in pin_assignment.md)

**HAL Initialization**:
```c
ETH_HandleTypeDef heth;

void MX_ETH_Init(void)
{
    heth.Instance = ETH;
    heth.Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
    heth.Init.Speed = ETH_SPEED_100M;
    heth.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
    heth.Init.PhyAddress = 0x01;
    heth.Init.RxMode = ETH_RXINTERRUPT;
    heth.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
    
    // DMA configuration
    heth.Init.DMABurstMode = ETH_DMABURST_LENGTH32;
    
    HAL_ETH_Init(&heth);
    
    // Enable RX/TX DMA
    HAL_ETH_EnableReceiveInterrupt(&heth);
}
```

**Interrupt Handler**:
```c
void ETH_IRQHandler(void)
{
    HAL_ETH_IRQHandler(&heth);
}

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
    // New Ethernet frame received
    // Forward to LwIP for processing
    ethernetif_input(&netif);
}
```

---

## <a id="spi"></a>SPI Driver (External Flash W25Q64)

**Configuration**:
- SPI1 (fastest)
- Clock: APB2 (84 MHz) / 2 = 42 MHz
- Mode: 0 (CPOL=0, CPHA=0) or Mode 3
- Data Width: 8 bits
- CS: GPIO control (PB14 typical)

**HAL Initialization**:
```c
SPI_HandleTypeDef hspi1;

void MX_SPI1_Init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  // 42MHz
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    
    HAL_SPI_Init(&hspi1);
}
```

**Flash Read/Write**:
```c
#define SPI_CS_PIN      GPIO_PIN_14
#define SPI_CS_PORT     GPIOB

void W25Q64_SelectChip(uint8_t select)
{
    if (select) {
        HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET);
    }
}

uint8_t W25Q64_ReadJEDID(void)
{
    uint8_t cmd = 0x9F;  // Read JEDID command
    uint8_t jedid[3];
    
    W25Q64_SelectChip(1);
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Receive(&hspi1, jedid, 3, 100);
    W25Q64_SelectChip(0);
    
    return jedid[0];
}
```

---

## <a id="i2c"></a>I2C Driver (EEPROM)

**Configuration**:
- I2C1: SCL PA8, SDA PB7
- Speed: 100 kHz (Standard) or 400 kHz (Fast)
- Addressing: 7-bit (0x50 for 24C02 EEPROM)

**HAL Initialization**:
```c
I2C_HandleTypeDef hi2c1;

void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;  // 100 kHz
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    HAL_I2C_Init(&hi2c1);
}
```

**EEPROM Read/Write**:
```c
#define EEPROM_ADDRESS   0x50     // 7-bit address
#define EEPROM_PAGE_SIZE 16       // 16 bytes per page

uint8_t EEPROM_Read(uint16_t addr, uint8_t *data, uint16_t len)
{
    uint8_t addr_bytes[2];
    addr_bytes[0] = (addr >> 8) & 0xFF;
    addr_bytes[1] = addr & 0xFF;
    
    // Write address
    if (HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDRESS << 1, addr_bytes, 2, 100) != HAL_OK) {
        return 1;  // Error
    }
    
    // Read data
    if (HAL_I2C_Master_Receive(&hi2c1, EEPROM_ADDRESS << 1, data, len, 100) != HAL_OK) {
        return 1;  // Error
    }
    
    return 0;  // Success
}

uint8_t EEPROM_Write(uint16_t addr, uint8_t *data, uint16_t len)
{
    uint8_t buffer[EEPROM_PAGE_SIZE + 2];
    
    buffer[0] = (addr >> 8) & 0xFF;
    buffer[1] = addr & 0xFF;
    memcpy(&buffer[2], data, len);
    
    // Write page
    if (HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDRESS << 1, buffer, len + 2, 100) != HAL_OK) {
        return 1;  // Error
    }
    
    HAL_Delay(5);  // Write cycle time
    return 0;  // Success
}
```

---

## <a id="adc"></a>ADC Driver (Sensors)

**Configuration**:
- ADC1: Sensor inputs
- Sample Rate: 1 MSPS
- Channels: Motor current, voltage, temperature
- DMA Transfer: Circular

**HAL Initialization**:
```c
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

void MX_ADC1_Init(void)
{
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;  // 84/4 = 21 MHz
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;             // Multiple channels
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.NbrOfDiscConversion = 0;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 3;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    
    HAL_ADC_Init(&hadc1);
    
    // Configure channels
    ADC_ChannelConfTypeDef sConfig = {0};
    
    // Channel 1: Motor Current (PA2)
    sConfig.Channel = ADC_CHANNEL_2;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // Channel 2: Voltage (PA3)
    sConfig.Channel = ADC_CHANNEL_3;
    sConfig.Rank = ADC_REGULAR_RANK_2;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    // Channel 3: Temperature (Internal sensor)
    sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank = ADC_REGULAR_RANK_3;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}
```

**DMA Configuration**:
```c
void MX_DMA_Init(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();
    
    hdma_adc1.Instance = DMA2_Stream0;
    hdma_adc1.Init.Channel = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    
    HAL_DMA_Init(&hdma_adc1);
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);
    
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}
```

**Start ADC**:
```c
uint16_t adc_buffer[3];  // Store 3 ADC values

void ADC_Start(void)
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer, 3);
}

void ADC_GetValues(uint16_t *current, uint16_t *voltage, uint16_t *temp)
{
    *current = adc_buffer[0];
    *voltage = adc_buffer[1];
    *temp = adc_buffer[2];
}
```

---

## <a id="pwm"></a>PWM/Timer Drivers

**TIM1** - Motor PWM Control (from LM298 driver)

Already implemented in `drivers/lm298.c`. Key points:
- 16-bit timer, 1 kHz frequency
- PWM on PA8 (TIM1_CH1)
- Direction on PA0, PA1

---

## <a id="gpio"></a>GPIO Driver

See BSP section (`docs/bsp_drivers.md`)

---

## <a id="dma"></a>DMA Configuration

### DMA Stream Allocation

| Stream | Purpose | Priority | Channel |
|--------|---------|----------|---------|
| DMA1_Stream0 | - | - | - |
| DMA1_Stream5 | UART TX | Medium | 4 |
| DMA1_Stream6 | UART RX | Medium | 4 |
| DMA2_Stream0 | ADC | High | 0 |
| DMA2_Stream2 | SPI RX | Medium | 3 |
| DMA2_Stream3 | SPI TX | Medium | 3 |
| DMA2_Stream4 | ETH RX | High | 0 |

### DMA Best Practices

1. **Alignment**: All buffers must be 4-byte aligned for DMA
2. **Placement**: Use main RAM for DMA buffers, NOT CCM
3. **Coherency**: Disable cache for DMA buffers or use cache operations
4. **Atomicity**: Access DMA buffers with interrupts disabled

```c
// Correct DMA buffer placement
uint16_t adc_buffer[64] __attribute__((aligned(4)));

// In linker script
.dma_buffers (NOLOAD) : {
    *(.dma_buffers)
} > RAM
```

---

## Error Handling

### Common Issues

| Error | Cause | Solution |
|-------|-------|----------|
| UART no RX | NVIC not enabled | Enable NVIC_EnableIRQ() |
| CAN bus off | No termination | Add 120Ω terminators |
| SPI wrong data | Clock phase/polarity | Check datasheet |
| ADC wrong value | Unaligned buffer | Align to 4 bytes |
| Ethernet no link | PHY address wrong | Verify schematic |

---

## Testing Peripherals

```c
void Test_All_Peripherals(void)
{
    // UART test
    printf("UART OK\n");
    
    // CAN test
    CAN_Transmit_Message(0x123, (uint8_t []){0xAA, 0xBB}, 2);
    
    // SPI test
    uint8_t jedid = W25Q64_ReadJEDID();
    printf("Flash JEDID: 0x%02X\n", jedid);
    
    // I2C test (if EEPROM present)
    // ...
    
    // ADC test
    uint16_t volt = adc_buffer[0];
    printf("ADC: %u\n", volt);
}
```

(Continued in next sections for Ethernet/LwIP, SD/FatFS, etc.)

# System Architecture

## Overview

This project demonstrates a real-time embedded firmware architecture using FreeRTOS on STM32F407.

The system is designed to simulate a motor-driven embedded platform where multiple subsystems interact in real-time.

The main focus is:

- task scheduling
- interrupt driven firmware
- DMA based data acquisition
- real-time data processing
- modular driver architecture

---

# System Block Diagram
# System Architecture

## Overview

This project demonstrates a real-time embedded firmware architecture using FreeRTOS on STM32F407.

The system is designed to simulate a motor-driven embedded platform where multiple subsystems interact in real-time.

The main focus is:

- task scheduling
- interrupt driven firmware
- DMA based data acquisition
- real-time data processing
- modular driver architecture

---

# System Block Diagram



Sensors
(ADC + DMA)
|
v
Sensor Task
|
RTOS Queue
|
Motor Control Task
|
PWM Driver
|
Motor

UART Telemetry
|
Comm Task
|
Host PC

System Monitor
|
Monitor Task



ADC
↓
DMA
↓
Ring Buffer
↓
Sensor Task
↓
RTOS Queue
↓
Motor Task

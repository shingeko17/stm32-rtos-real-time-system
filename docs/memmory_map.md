# Memory Map

## Overview

Efficient memory usage is critical in embedded systems.

This document describes how RAM and Flash memory are allocated in the project.

---

# STM32F407 Memory

Flash: 1 MB  
RAM: 192 KB

Memory regions:

Flash memory  
SRAM1  
SRAM2  
Peripheral memory

---

# Firmware Memory Layout

Flash
│
├ Boot code
├ Application code
└ Constant data

RAM
│
├ RTOS heap
├ Task stacks
├ Driver buffers
├ DMA buffers
└ Ring buffers


---

# RTOS Heap

FreeRTOS uses a heap for dynamic allocation.

Typical usage:

- queue allocation
- semaphore allocation
- task creation

Heap size example:

20 KB

---

# Task Stack Allocation

Each task requires its own stack.

Example:

motor_task stack: 512 bytes  
sensor_task stack: 1024 bytes  
comm_task stack: 1024 bytes  
monitor_task stack: 512 bytes  

---

# DMA Buffers

DMA buffers must be aligned properly.

Example:

ADC DMA buffer
uint16_t adc_buffer[64];


uint16_t adc_buffer[64];



Alignment ensures efficient transfer.

---

# Ring Buffers

Ring buffers are used for non-blocking data transfer between tasks.

Example usage:

UART RX buffer  
Sensor data pipeline  

Advantages:

- no blocking
- efficient memory reuse

---

# Memory Safety

Key practices:

- avoid large stack usage
- check heap usage
- avoid memory fragmentation

---

# Future Improvements

Potential enhancements:

- static memory allocation
- memory usage monitoring
- stack overflow detection
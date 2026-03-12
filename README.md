# STM32 RTOS Real-Time Motor System

## Overview

This project demonstrates a real-time embedded system architecture using an RTOS on STM32F407.

The goal of the project is not motor control algorithm research, but rather to explore **embedded system design**, including:

- RTOS task scheduling
- interrupt-driven architecture
- DMA based data pipeline
- real-time data processing
- modular firmware architecture

The motor is used as a workload to evaluate system timing behavior and task scheduling performance.

---

## Hardware

- MCU: STM32F407VET6
- Motor: DC motor with encoder
- Interface: UART telemetry
- Sensors: ADC inputs

---

## Software Architecture

The firmware is structured in multiple layers:

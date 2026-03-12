
Legend:

M = motor task execution  
S = sensor task execution  
C = communication task  
m = monitor task  

---

# Scheduling Strategy

The RTOS scheduler uses **priority-based preemptive scheduling**.

Priority order:

motor_task > sensor_task > comm_task > monitor_task

This ensures that the motor control loop always meets its 1 ms deadline.

---

# Worst Case Execution Time (WCET)

Estimated execution time of each task:

| Task | WCET |
|-----|-----|
| motor_task | 50 us |
| sensor_task | 200 us |
| comm_task | 500 us |
| monitor_task | 300 us |

Total CPU usage remains well below 100%, ensuring real-time safety margin.

---

# Interrupt Interaction

Some events are triggered by hardware interrupts:

ADC DMA complete interrupt  
UART receive interrupt  
Timer interrupt (system tick)

Interrupt handlers perform minimal work and notify tasks using RTOS primitives.

---

# Latency Measurement

System latency can be measured using:

- GPIO toggle + oscilloscope
- cycle counter (DWT)
- timestamp logging via UART

This allows verification of real-time performance.

---

# Design Principles

The timing design follows several embedded system best practices:

- avoid blocking operations
- keep interrupt handlers short
- use queues for task communication
- prioritize time-critical tasks

---

# Future Work

Potential improvements include:

- deadline monitoring
- runtime profiling
- advanced scheduling analysis
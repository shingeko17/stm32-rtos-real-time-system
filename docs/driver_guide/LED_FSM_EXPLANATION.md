# LED FSM Implementation - Giải thích & So sánh

## 📊 So sánh: Cách cũ vs FSM

### **Cách cũ (Blocking)**
```c
void LED_Running_Light(void) {
    for (i = 0; i < LED_COUNT; i++) {
        LED_On(i);
        delay_ms(500);  // ⛔ BLOCK HỆ THỐNG ở đây
    }
    for (i = 0; i < LED_COUNT; i++) {
        LED_Off(i);
        delay_ms(500);  // ⛔ BLOCK HỆ THỐNG ở đây
    }
}
```

**Vấn đề:**
- ⛔ Hệ thống bị "đóng băng" trong 8 * 500ms = 4 giây
- ❌ Không thể xử lý các sự kiện khác (UART commands, sensor data)
- ❌ Không phù hợp với RTOS (cắt giảm hiệu năng các task khác)

---

### **FSM (Non-blocking) **
```c
void LED_FSM_Execute(led_fsm_t *fsm) {
    led_fsm_event_t event = LED_FSM_GetEvent(fsm);  // Lấy event
    
    switch (fsm->state) {
        case LED_STATE_ON_SEQUENCE:
            if (event == LED_EVENT_TIMEOUT) {
                LED_On(fsm->led_index++);  // ✅ Chỉ bật 1 LED
                // ✅ Không block, trở lại ngay lập tức
            }
            break;
        // ...
    }
}
```

**Lợi ích:**
- ✅ Không block (mất <1ms)
- ✅ Có thể gọi trong vòng lặp RTOS mà không ảnh hưởng
- ✅ Dễ thêm features (pause, reverse, speed control)

---

## 🔄 FSM Flow Diagram

```
┌─────────────────────────────────────────────────────────┐
│                  LED_FSM_Execute()                      │
│              (gọi từ main loop, 1ms/lần)                │
└─────────────────────────────────────────────────────────┘
                            │
                            ▼
                 ┌──────────────────────┐
                 │  Get current event   │
                 │ (timer, state check) │
                 └──────────────────────┘
                            │
                 ┌──────────▼──────────┐
                 │  State Machine      │
                 │  (3 states)         │
                 └──────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        ▼                   ▼                   ▼
   ┌─────────┐      ┌──────────────┐      ┌─────────────┐
   │  IDLE   │      │ ON_SEQUENCE  │      │ OFF_SEQUENCE│
   └─────────┘      └──────────────┘      └─────────────┘
        │                   │                   │
   START→              TIMEOUT→          TIMEOUT→
        │                LED_On()             LED_Off()
   SEQUENCE_DONE←       DONE when all 8 LED done
        │                   │
        └───────────────────┘
```

---

## 📝 3 States Giải thích

| State | Mô tả | Transition |
|-------|-------|-----------|
| **IDLE** | Chờ lệnh bắt đầu | → ON_SEQUENCE khi START |
| **ON_SEQUENCE** | Bật LED lần lượt (0→7) | → OFF_SEQUENCE khi bật xong 8 LED |
| **OFF_SEQUENCE** | Tắt LED lần lượt (0→7) | → IDLE khi tắt xong 8 LED |

---

## ⚙️ Event Types

| Event | Kích hoạt | Hành động |
|-------|-----------|----------|
| `LED_EVENT_START` | Manual trigger | Bắt đầu sequence |
| `LED_EVENT_TIMEOUT` | Timer >= 500ms | Di chuyển sang LED tiếp theo |
| `LED_EVENT_SEQUENCE_DONE` | Đã xử lý 8 LED | Chuyển sang state tiếp theo |

---

## 🎯 Cách sử dụng

### **Khởi tạo FSM:**
```c
led_fsm_t led_fsm;
LED_FSM_Init(&led_fsm);
```

### **Vòng lặp chính (Main loop):**
```c
while (1) {
    /* Mỗi 1ms: */
    LED_FSM_Update(&led_fsm, 1);    // Cập nhật timer
    LED_FSM_Execute(&led_fsm);      // Thực thi state machine
    
    // ✅ Lúc này hệ thống vẫn có thể làm việc khác!
    // Ví dụ: đọc UART, xử lý sensor, ...
}
```

### **Trong RTOS (với FreeRTOS):**
```c
/* Task chạy mỗi 1ms */
void LEDTask(void *pvParameters) {
    led_fsm_t led_fsm;
    LED_FSM_Init(&led_fsm);
    
    while (1) {
        LED_FSM_Update(&led_fsm, 1);
        LED_FSM_Execute(&led_fsm);
        vTaskDelay(pdMS_TO_TICKS(1));  // Chỉ block 1ms
    }
}
```

---

## 🚀 Mở rộng FSM (Example)

### **Thêm PAUSE state:**
```c
typedef enum {
    LED_STATE_IDLE,
    LED_STATE_ON_SEQUENCE,
    LED_STATE_OFF_SEQUENCE,
    LED_STATE_PAUSE          // ← Mới
} led_fsm_state_t;
```

### **Thêm sự kiện điều khiển:**
```c
typedef enum {
    LED_EVENT_START,
    LED_EVENT_TIMEOUT,
    LED_EVENT_SEQUENCE_DONE,
    LED_EVENT_PAUSE,         // ← Mới
    LED_EVENT_RESUME         // ← Mới
} led_fsm_event_t;
```

### **Xử lý UART commands:**
```c
// Từ UART command, gửi events
if (UART2_DataAvailable()) {
    char cmd = UART2_RecvChar();
    if (cmd == 'P') {
        led_fsm.state = LED_STATE_PAUSE;  // Pause
    }
    if (cmd == 'R') {
        led_fsm.state = LED_STATE_ON_SEQUENCE;  // Resume
    }
}
```

---

## 📈 FSM vs Non-FSM

| Tiêu chí | Non-FSM (Blocking) | FSM (Non-blocking) |
|---------|-------------------|-------------------|
| **Code rõ ràng** | ❌ Ẩn state trong procedural code | ✅ Explicit state transitions |
| **Blocking** | ⛔ Có | ✅ Không |
| **Dễ debug** | ❌ Khó track state | ✅ Dễ, state rõ ràng |
| **Dễ mở rộng** | ❌ Phải sửa logic | ✅ Thêm state/event dễ dàng |
| **RTOS-friendly** | ❌ Không | ✅ Có |
| **Code dài** | ✅ Ngắn |  ❌ Dài hơn (nhưng rõ ràng) |

---

## 🎓 Takeaway

**FSM không phải lúc nào cũng cần**, nhưng khi:
- ✅ Có nhiều **states** rõ ràng
- ✅ **Transitions** không tuyến tính
- ✅ Cần **non-blocking** operation
- ✅ Muốn **dễ bảo trì** & **mở rộng**

→ **FSM là choice tốt!**

Đối với LED control: FSM cải thiện tính **linh hoạt** & **responsiveness** của hệ thống.

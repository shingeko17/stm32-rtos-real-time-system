# BUILD_HANDOVER.md

Hướng dẫn Di chuyển Build System - Từ Bash Scripts sang Makefile

---

## Tổng quan

Tài liệu này hướng dẫn di chuyển từ **bash scripts** (`build.sh`, `flash.sh`) sang **Makefile** chuẩn hóa khi dự án đạt ổn định. Đây là cho **chuyển giao cho team khác** hoặc **triển khai production**.

---

## Giai đoạn Phát triển (Hiện tại)

### Tại sao dùng bash scripts bây giờ?
- ✅ **Dễ debug** - Mỗi bước rõ ràng và có thể kiểm soát
- ✅ **Phát triển nhanh** - Lặp lại nhanh chóng trong quá trình phát triển
- ✅ **Linh hoạt tùy chỉnh** - Dễ thêm/bớt bước
- ✅ **Dễ tiếp cận** - Team mới trong embedded systems

### Quy trình hiện tại:
```bash
# Bước 1: Build firmware
./build.sh rebuild

# Bước 2: Nạp vào board
./flash.sh
```

### Các file liên quan:
- `build.sh` - Dịch mã ARM GCC → firmware.hex
- `flash.sh` - Nạp firmware qua OpenOCD → board

---

## Giai đoạn Chuyển giao (Tương lai)

### Tại sao di chuyển sang Makefile?

| Tiêu chí | bash scripts | Makefile |
|----------|-------------|----------|
| **Chuẩn hóa** | Tùy chỉnh | Tiêu chuẩn ngành (GNU Make) |
| **Đường cong học tập** | Vừa phải | Dốc (nhưng đáng giá) |
| **Bảo trì** | Scripts phân tán | Tập trung trong 1 file |
| **Khả năng mở rộng** | Khó bảo trì (nhiều files) | Dễ (tất cả trong 1 Makefile) |
| **Chuyển giao team** | Cần giải thích mỗi script | Tiêu chuẩn - bất kỳ dev nào cũng hiểu |
| **Dependency** | Theo dõi thủ công | Tự động theo dõi |
| **Tái tạo lại** | Phụ thuộc logic script | Đảm bảo bởi Make |

### Khi nào nên di chuyển?

**Chuyển sang Makefile khi:**
1. ✅ LED blink firmware hoạt động
2. ✅ Motor driver tích hợp và ổn định (Phase 2-3)
3. ✅ Tất cả tính năng đã test và không crash
4. ✅ Dự án sẵn sàng chuyển giao/demo
5. ✅ Tài liệu hoàn chỉnh

**KHÔNG di chuyển sớm** - Giữ bash scripts trong giai đoạn phát triển tích cực.

---

## Tham chiếu Lệnh Makefile

Khi chuyển sang Makefile, sử dụng các lệnh này:

### Build

```bash
# Build firmware (dịch mới)
make all

# Giống trên (ngắn hơn)
make
```

**Output:**
```
[CC] Compiling: application/minimal_led_blink.c
[CC] Compiling: drivers/system_init/system_stm32f4xx.c
...
[LD] Linking: build/firmware/STM32F407_MotorControl.elf
[OBJCOPY] Converting to HEX: build/firmware/STM32F407_MotorControl.hex
[SUCCESS] Firmware: build/firmware/STM32F407_MotorControl.elf
```

### Clean

```bash
# Xóa tất cả build objects (an toàn chạy bất kỳ lúc nào)
make clean
```

**Kết quả:** Xóa folder `build/`, sẵn sàng dịch lại từ đầu.

### Flash

```bash
# Nạp firmware vào STM32F407 qua ST-Link
make flash
```

**Output:**
```
[FLASH] Programming STM32F407 via ST-Link...
Info : ST-Link detected
Info : stm32f4x.cpu: hardware has 6 breakpoints, 4 watchpoints
...
[SUCCESS] Flashing complete!
```

### Size

```bash
# Hiển thị kích thước firmware (RAM, Flash)
make size
```

**Output:**
```
[SIZE] Firmware memory usage:
   text    data     bss     dec     hex filename
  12345    1024    2048  15417   3C39 build/firmware/STM32F407_MotorControl.elf

File sizes:
  1.2 KB - firmware.hex
  1.0 KB - firmware.bin
```

### Reset

```bash
# Reset STM32 mà không lập trình lại
make reset
```

**Dùng khi:** Khởi động lại firmware sau khi mất điện, không cần nạp lại.

### Help

```bash
# Hiển thị tất cả lệnh có sẵn
make help
```

---

## Danh sách Check Di chuyển

### Trước khi chuyển sang Makefile:

- [ ] Dự án compile không lỗi trong build.sh
- [ ] Firmware nạp và hoạt động qua flash.sh
- [ ] Tất cả unit tests pass
- [ ] Không có segfaults hoặc hangs
- [ ] Tài liệu đã cập nhật (file này + files khác)
- [ ] Team members đã được đào tạo Makefile

### Bước di chuyển:

1. **Thông báo chuyển đổi**
   ```
   "Dự án hiện dùng Makefile để chuẩn hóa"
   ```

2. **Giữ bash scripts làm backup**
   - Archive `build.sh` và `flash.sh` vào folder riêng
   - Tham khảo: `docs/BUILD_HANDOVER.md` - "Emergency Recovery"

3. **Cập nhật README.md**
   ```markdown
   ## Build & Nạp
   
   ### Quick Start
   
   ```bash
   make              # Build firmware
   make flash        # Nạp vào STM32F407
   ```
   
   Chi tiết: `make help`
   ```

4. **Cập nhật CI/CD pipelines** (nếu có)
   - Thay `./build.sh` bằng `make`
   - Thay `./flash.sh` bằng `make flash`

5. **Thông báo team**
   - Hướng đến tài liệu này
   - Hướng dẫn lệnh Makefile

---

## So sánh: Quy trình bash vs Makefile

### Phát triển (Hiện tại - bash)

```bash
# 1. Build lại từ đầu
./build.sh rebuild

# 2. Nạp
./flash.sh

# 3. Kết quả
# ✅ Firmware: 2.8KB HEX
# [SUCCESS] Flashing complete
```

### Production (Tương lai - Makefile)

```bash
# 1. Clean và build (1 lệnh)
make clean && make

# 2. Nạp
make flash

# 3. Kết quả (giống bash - output y hệt)
# ✅ Firmware: 2.8KB HEX
# [SUCCESS] Flashing complete
```

---

## Khôi phục Acency: Quay lại bash scripts

**Nếu Makefile gặp vấn đề**, quay lại nhanh:

### Bước 1: Tìm backup bash scripts
```bash
# Nếu lưu trữ ở location riêng
cp docs/backup/build.sh .
cp docs/backup/flash.sh .
```

### Bước 2: Dùng bash scripts
```bash
bash build.sh rebuild
bash flash.sh
```

### Bước 3: Báo cáo vấn đề
Ghi lại cái gì bị lỗi trong Makefile và sửa.

---

## Cấu trúc Makefile (Tham khảo Kỹ thuật)

### Các phần chính trong Makefile:

```makefile
# Tên dự án
PROJECT = STM32F407_MotorControl

# Công cụ compiler (ARM GCC)
CC = arm-none-eabi-gcc

# Cờ compiler (tối ưu, cảnh báo, debug)
CFLAGS = -mcpu=cortex-m4 -mthumb -O2 -g3

# Các file source
C_SOURCES = application/minimal_led_blink.c drivers/...

# Build rules
all: $(FIRMWARE_DIR)/$(PROJECT).elf $(FIRMWARE_DIR)/$(PROJECT).hex

# Flash target
flash: $(FIRMWARE_DIR)/$(PROJECT).hex
	$(OPENOCD) ... program $(FIRMWARE_DIR)/$(PROJECT).hex ...

# Clean target
clean:
	rm -rf $(BUILD_DIR)
```

### Makefile targets dùng trong dự án này:

| Target | Tác dụng | bash tương đương |
|--------|---------|------------------|
| `make` / `make all` | Build | `./build.sh` |
| `make clean` | Xóa build/ | N/A |
| `make flash` | Nạp firmware | `./flash.sh` |
| `make size` | Xem memory | N/A |
| `make reset` | Reset STM32 | N/A |

---

## Danh sách Check Chuyển giao Team

### Cho team members mới nhận dự án:

- [ ] Clone/download dự án
- [ ] Chạy `make help` xem có lệnh nào
- [ ] Đọc README.md để nắm tổng quan
- [ ] Đọc file này (**BUILD_HANDOVER.md**)
- [ ] Đọc [docs/clockconfig.md](clockconfig.md) cho cài đặt clock
- [ ] Đọc [docs/linkledver2.md](linkledver2.md) cho spec LED firmware
- [ ] Thử: `make && make flash` để xác minh setup
- [ ] Check board bằng mắt (LEDs nhấp nháy = thành công)

### Cho kỹ sư chuyển giao:

- [ ] Đảm bảo dự án build sạch: `make clean && make`
- [ ] Đảm bảo firmware nạp: `make flash`
- [ ] Hướng team đến tài liệu này
- [ ] Hướng dẫn quy trình build
- [ ] Để team làm lần build/flash đầu tiên độc lập
- [ ] Trả lời câu hỏi

---

## Chuyển qua lại: Makefile → bash (nếu cần)

**Cho các trường hợp đặc biệt** (CI/CD, automation, custom builds):

1. Giữ Makefile làm chính
2. Dùng bash scripts cho logic tùy chỉnh
3. Ghi lại custom workflow trong file .md riêng

Ví dụ:
```bash
# Dùng Makefile cho build chuẩn
make

# Dùng bash custom cho tác vụ đặc biệt
bash custom_test.sh
```

---

## FAQ

### Q: Có thể dùng cả bash scripts và Makefile?
**A:** Được, nhưng không khuyến cáo. Chọn một trong:
- **Phát triển**: bash scripts (linh hoạt)
- **Production**: Makefile (chuẩn)

### Q: Nếu Makefile bị lỗi?
**A:** 
1. Check error: `make 2>&1 | tail -20`
2. Thử clean rebuild: `make clean && make`
3. Xác minh ARM GCC: `arm-none-eabi-gcc --version`
4. Quay lại bash: `./build.sh && ./flash.sh`

### Q: Có thể tùy chỉnh Makefile?
**A:** Được! Edit rules trong Makefile (nhưng giữ backup trước):
```bash
cp Makefile Makefile.bak    # Backup
# Edit Makefile theo ý
make clean && make           # Test
```

### Q: Makefile có cross-platform?
**A:** Bán-compatible (cmd Windows vs Unix shells khác). Dùng:
- **Linux/macOS**: Hoàn hảo
- **Windows Cmd**: Có thể cần sửa (dùng Git Bash thay thế)
- **Windows PowerShell**: Không khuyến cáo (dùng Bash)

### Q: Khi nào cập nhật file này?
**A:** Sau:
- Thay đổi kiến trúc lớn
- Nâng cấp tool (compiler, OpenOCD)
- Cải tiến quy trình
- Feedback từ team

---

## Bước tiếp theo

1. **Ngắn hạn** (HIỆN TẠI): Dùng bash scripts, focus vào LED blink
2. **Trung hạn** (Tuần 2-3): Tích hợp motor driver
3. **Dài hạn** (Trước chuyển giao): Chuyển sang Makefile
4. **Lúc chuyển giao**: File này + Makefile là deliverable cho team

---

## Tài liệu liên quan

- [README.md](../README.md) - Tổng quan dự án
- [docs/linkledver2.md](linkledver2.md) - Spec LED firmware
- [docs/clockconfig.md](clockconfig.md) - Hướng dẫn cài đặt clock
- [ERRORS_AND_LESSONS/](../ERRORS_AND_LESSONS/) - Bài học rút ra

---

## Lịch sử tài liệu

| Version | Ngày | Thay đổi |
|---------|------|---------|
| 1.0 | 2026-03-24 | Version đầu - hướng dẫn di chuyển bash → Makefile |

---

**Cập nhật lần cuối:** 2026-03-24  
**Được duy trì bởi:** STM32 Motor Control Team  
**Trạng thái:** Giai đoạn Phát triển Tích cực (giữ bash scripts)

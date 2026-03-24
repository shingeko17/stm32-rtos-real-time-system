# linkled.md

## Mục đích
File này chỉ phục vụ **test giao tiếp toolchain ↔ board** bằng blink LED, không phải mục tiêu tổng của dự án.

## Cấu hình test LED
- LED1: `PE13`
- LED2: `PE14`
- LED3: `PE15`
- Boot base: `0x08000000`

## Kỳ vọng
- Sau reset: có nhịp boot LED ngắn.
- Sau đó: 3 LED blink cùng nhau theo chu kỳ.

## Build nhanh
```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Rebuild
```

## Flash nhanh
```powershell
powershell -ExecutionPolicy Bypass -File .\flash_latest.ps1
```

# LED Bring-up (LED1/LED2/LED3)

## Muc tieu
- Xac nhan toolchain, build pipeline, va ket noi flash den board.
- Xac nhan firmware boot tu `0x08000000`.
- Xac nhan 3 LED tren board hoat dong dung chan cau hinh.

## Trang thai trien khai hien tai
- Da co startup + vector table trong `BOOT/stm.s`.
- Da co linker script boot base `0x08000000` trong `BOOT/stm.ld`.
- Da co ung dung LED test trong `application/minimal_led_blink.c`.
- Da co build automation trong `build.ps1` (dang build LED-focused).

## Cau hinh LED hien tai
- LED1: `PE13`
- LED2: `PE14`
- LED3: `PE15`
- Logic hien tai: ghi BSRR bit set de bat LED, bit reset de tat LED.

## Hanh vi firmware ky vong
1. Sau reset: boot pulse ngan tai `Reset_Handler`.
2. Vao `main()`: 3 LED blink dong thoi theo chu ky 500ms ON / 500ms OFF.

## Lenh build
```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Rebuild
```

## Lenh flash
```powershell
powershell -ExecutionPolicy Bypass -File .\flash_latest.ps1
```

## Tieu chi pass/fail
- PASS:
  - Flash thanh cong khong bao loi verify.
  - Sau reset thay boot pulse ngan.
  - Sau do 3 LED nhay dong bo on/off.
- FAIL:
  - Khong ket noi duoc ST-Link/OpenOCD.
  - Flash loi hoac verify loi.
  - LED sang nguoc logic, sai chan, hoac khong nhay.

## Spec/Doc con thieu (can ban cung cap, khong gia dinh)
1. Xac nhan board LED la active-high hay active-low.
2. Xac nhan ten board/chip chinh xac dang test (STM32F407 variant cu the).
3. Xac nhan file OpenOCD config chuan cua ban (hien dang dung `stm32f4.cfg`).
4. Xac nhan muc tieu chu ky blink:
   - Dung 500ms/500ms hay doi sang pattern khac?
5. Xac nhan co can giu boot pulse o startup hay bo di.

## Buoc tiep theo sau khi ban xac nhan spec
1. Chot logic bat/tat LED theo active-high/active-low.
2. Chot blink pattern.
3. Flash tren board va ghi ket qua hardware test.

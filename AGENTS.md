# Desk Clock Agent Rules

When assisting with this project, strictly adhere to these hardware constraints and architectural rules:

## 1. SPI Isolation Rule
**DO NOT** attempt to put the MAX7219 matrices and the ST7735 TFT display on the same SPI bus. 
The MAX7219 modules run on 5V logic and are extremely noisy, which corrupts the ST7735's delicate 3.3V signals.
- **MAX7219** MUST use Software SPI (Bitbang) on GPIO 17, 18, and 23.
- **ST7735** MUST use Native Hardware SPI (`spidev`) on SPI0.

## 2. TFT Chip Select Rule
**DO NOT** attempt to manually toggle the Chip Select (CS) pin for the ST7735 in code.
The Linux kernel is configured via `/boot/config.txt` (`dtoverlay=spi0-1cs,cs0_pin=22`) to natively map SPI0 CS0 to GPIO 22. 
If you add `GPIO_SET`/`GPIO_CLR` calls for the CS pin, you will conflict with the kernel driver and corrupt the SPI timing.

## 3. Font Rendering Rule
This project deliberately avoids external TTF rendering libraries (like Pillow/PIL) at runtime for performance and simplicity.
All text is rendered via pre-rasterized bitmap font arrays in the C source:
- `FONT_5X7` in `src/fonts.h` — for small labels and temperature unit text
- `FONT_8X12` in `src/font_8x12.h` — for humidity text (generated from Consolas TTF via Pillow at build time)
- `FONT_32X48` in `src/fonts.h` — for large temperature digits
If you add new features that display text, ensure all text is uppercased and strictly filtered to only include characters that exist in the relevant font array, otherwise the program will crash with an out-of-bounds read.

## 4. GPIO Access Rule
**DO NOT** use sysfs (`/sys/class/gpio/`) for GPIO access. Both `tft.c` and `max7219.c` use direct register access via `/dev/gpiomem` mmap. This eliminates the need for `sudo` and avoids sysfs deprecation issues on modern kernels.

## 5. Thread Safety Rule
**DO NOT** use `localtime()` or `gmtime()` — they return pointers to shared static buffers and are not thread-safe. Always use `localtime_r()` and `gmtime_r()` with local `struct tm` buffers.

## 6. Display Toggling Rule
**DO NOT** modify or remove the hardware button polling on `GPIO 3` (Pin 5) or the software `SIGUSR1` toggle logic in `main.c`. **DO NOT** revert the TFT LED backlight pin to a hardwired 3.3V power rail; it must remain mapped to `GPIO 27` (Pin 13) to allow the C program to cycle through the 4 display power states.

## 7. Configuration Updates Rule
When adding a new configurable feature to the deskclock, you MUST ALWAYS update `src/config.cfg.default` and `src/constants.h` to include the new configuration setting and its default value.

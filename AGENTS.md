# Desk Clock Agent Rules

When assisting with this project, strictly adhere to these hardware constraints and architectural rules:

## 1. SPI Isolation Rule
**DO NOT** attempt to put the MAX7219 matrices and the ST7735 TFT display on the same SPI bus. 
The MAX7219 modules run on 5V logic and are extremely noisy, which corrupts the ST7735's delicate 3.3V signals.
- **MAX7219** MUST use Software SPI (Bitbang) on GPIO 17, 18, and 23.
- **ST7735** MUST use Native Hardware SPI (`spidev`) on SPI0.

## 2. TFT Chip Select Rule
**DO NOT** attempt to manually toggle the Chip Select (CS) pin for the ST7735 in Python.
The Linux kernel is configured via `/boot/config.txt` (`dtoverlay=spi0-1cs,cs0_pin=22`) to natively map SPI0 CS0 to GPIO 22. 
If you add `GPIO.output()` calls for the CS pin, you will conflict with the kernel driver and corrupt the SPI timing.

## 3. Font Rendering Rule
This project deliberately avoids external TTF rendering libraries (like Pillow/PIL) for performance and simplicity.
All text is rendered via the custom `FONT_5x7` pixel dictionary in `clock.py`.
If you add new features (e.g. displaying new weather conditions or statuses), ensure all text is `.upper()` and strictly filtered to only include characters that exist in the `FONT_5x7` dictionary, otherwise the script will crash.

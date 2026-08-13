# Raspberry Pi Desk Clock & Weather Dashboard

A smart desk clock powered by a Raspberry Pi 1, featuring dual MAX7219 LED matrices for multi-timezone clocks (Local & Manila) and a vibrant ST7735 TFT display serving as a live Vancouver weather dashboard with animated weather scenes.

## Features

- **Dual-timezone LED clock** — Local time (top) and Manila/UTC+8 (bottom) on daisy-chained 7-segment displays
- **Live weather dashboard** — Temperature with gradient 32x48 font, humidity with gradient 8x12 font
- **Animated weather scenes** — Sun with rotating rays, twinkling stars with shooting comets, teardrop rain, geometric snowflakes, bouncing clouds
- **Pure C implementation** — Near-zero startup, minimal RAM/CPU, no Python runtime overhead
- **No sudo required** — Direct `/dev/gpiomem` register access

## Hardware Configuration

This project uses a split-SPI architecture to prevent electrical interference between the 5V LED matrices and the delicate 3.3V TFT screen.

### 1. Dual MAX7219 LED Matrices (Timezones)
Driven via **Software SPI (Bitbang)** to isolate their noisy 5V logic from the hardware SPI bus.
* **DIN (Data In):** GPIO 17 (Pin 11)
* **CLK (Clock):** GPIO 18 (Pin 12)
* **CS (Chip Select):** GPIO 23 (Pin 16)
* **VCC:** 5V (Pin 2)
* **GND:** Ground (Pin 6)

### 2. ST7735 TFT Display (Weather Dashboard)
Driven via **Hardware SPI (spidev)** for blazing-fast screen clears and drawing.
* **SDA/MOSI:** GPIO 10 (Pin 19 - Hardware SPI0 MOSI)
* **SCK/CLK:** GPIO 11 (Pin 23 - Hardware SPI0 SCLK)
* **A0/DC (Data/Command):** GPIO 24 (Pin 18)
* **RESET:** GPIO 25 (Pin 22)
* **CS (Chip Select):** GPIO 22 (Pin 15)
* **LED (Backlight):** 3.3V (Pin 1)
* **VCC:** 3.3V (Pin 17)
* **GND:** Ground (Pin 20)

## ⚠️ Critical Boot Configuration

To enable Hardware SPI for the TFT screen without conflicting with the Linux kernel, you **MUST** configure the Raspberry Pi's bootloader to natively map the SPI Chip Select signal to GPIO 22.

Add the following lines to your `/boot/config.txt`:

```ini
dtparam=spi=on
dtoverlay=spi0-1cs,cs0_pin=22
```
*Note: You must reboot the Pi after making this change!*

## Building & Running

### Prerequisites (DietPi/Debian)
```bash
sudo apt install build-essential libcurl4-openssl-dev
```

### Build
```bash
cd src
make clean && make
```

### Run
```bash
./deskclock
```

The clock will start immediately, showing the time on the LED matrices and fetching live weather data from [wttr.in](https://wttr.in) every 10 minutes.

## Project Structure

```
├── src/
│   ├── main.c          # Main 10 FPS loop, LED + TFT orchestration
│   ├── tft.c / tft.h   # ST7735 SPI driver, framebuffer, text rendering
│   ├── max7219.c / .h  # MAX7219 bitbang driver via /dev/gpiomem
│   ├── weather.c / .h  # Background thread fetching weather via libcurl
│   ├── anim.c / anim.h # Weather animations (sun, rain, snow, clouds, stars)
│   ├── fonts.h         # 5x7 + 32x48 bitmap font arrays
│   ├── font_8x12.h     # 8x12 Consolas-derived font for humidity
│   └── Makefile
├── legacy/clock.py       # Original Python implementation (reference)
└── AGENTS.md           # AI agent development rules
```

## Legacy Python Version

The original Python implementation is preserved in `legacy/clock.py` for reference. It requires `python3-spidev` and `python3-rpi.gpio`. The C version supersedes it with significantly better performance.

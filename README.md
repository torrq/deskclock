# Raspberry Pi Desk Clock & Weather Dashboard

A smart desk clock powered by a Raspberry Pi 1, featuring dual MAX7219 LED matrices for multi-timezone clocks (Local & Manila) and a vibrant ST7735 TFT display serving as a live Vancouver weather dashboard.

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

Add the following line to your `/boot/config.txt` (or `/boot/firmware/config.txt` depending on your OS):

```ini
dtparam=spi=on
dtoverlay=spi0-1cs,cs0_pin=22
```
*Note: You must reboot the Pi after making this change!*

## Running the Clock

Install the required dependencies (`spidev`, `RPi.GPIO`):
```bash
sudo apt-get install python3-spidev python3-rpi.gpio
```

Start the dashboard:
```bash
python3 src/clock.py
```
The script will run indefinitely, querying `wttr.in` every 15 minutes for live weather data.

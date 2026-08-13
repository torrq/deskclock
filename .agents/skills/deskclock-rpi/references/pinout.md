# Hardware Pinout & Wiring Reference

## Raspberry Pi 1 Model B (BCM2835)

### GPIO Header Map (relevant pins)

```
                    3V3 (1) (2)  5V
                  GPIO2 (3) (4)  5V       ← MAX7219 VCC
                  GPIO3 (5) (6)  GND      ← MAX7219 GND
                  GPIO4 (7) (8)  GPIO14
                    GND (9) (10) GPIO15
    MAX7219 DIN → GPIO17(11)(12) GPIO18   ← MAX7219 CLK
                 GPIO27(13)(14) GND
    TFT CS      → GPIO22(15)(16) GPIO23   ← MAX7219 CS
          3V3   → (17)(18) GPIO24         ← TFT DC
    TFT MOSI    → GPIO10(19)(20) GND      ← TFT GND
                  GPIO9 (21)(22) GPIO25   ← TFT RST
    TFT SCLK    → GPIO11(23)(24) GPIO8
                    GND (25)(26) GPIO7
```

### MAX7219 Wiring (Software SPI)

| MAX7219 Pin | RPi GPIO | RPi Pin | Notes |
|---|---|---|---|
| VCC | — | Pin 2 | 5V power |
| GND | — | Pin 6 | Ground |
| DIN | GPIO 17 | Pin 11 | Data In (bitbang) |
| CLK | GPIO 18 | Pin 12 | Clock (bitbang) |
| CS | GPIO 23 | Pin 16 | Chip Select (bitbang) |

Two MAX7219 modules daisy-chained: DOUT of first → DIN of second.

### ST7735 TFT Wiring (Hardware SPI)

| TFT Pin | RPi GPIO | RPi Pin | Notes |
|---|---|---|---|
| VCC | — | Pin 17 | 3.3V power |
| GND | — | Pin 20 | Ground |
| SDA/MOSI | GPIO 10 | Pin 19 | SPI0 MOSI |
| SCK/CLK | GPIO 11 | Pin 23 | SPI0 SCLK |
| CS | GPIO 22 | Pin 15 | Kernel-managed via dtoverlay |
| A0/DC | GPIO 24 | Pin 18 | Data/Command (mmap) |
| RESET | GPIO 25 | Pin 22 | Reset (mmap) |
| LED | — | Pin 1 | 3.3V backlight |

## Boot Configuration

Required in `/boot/config.txt`:

```ini
dtparam=spi=on
dtoverlay=spi0-1cs,cs0_pin=22
```

This tells the kernel to manage SPI0 CS0 on GPIO 22 natively.
**Do not** toggle GPIO 22 manually in code.

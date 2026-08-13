import spidev
import time

# Open SPI bus
spi = spidev.SpiDev()
spi.open(0, 0)  # (bus, device)
spi.max_speed_hz = 1000000

# Number of 8-digit displays (each display consists of two 4-digit modules)
num_displays = 4

# 7-segment encoding for digits 0-9
segment_map = {
    0: 0b01111110,  # 0
    1: 0b00110000,  # 1
    2: 0b01101101,  # 2
    3: 0b01111001,  # 3
    4: 0b00110011,  # 4
    5: 0b01011011,  # 5
    6: 0b01011111,  # 6
    7: 0b01110000,  # 7
    8: 0b01111111,  # 8
    9: 0b01111011   # 9
}

# Initialize the MAX7219
def max7219_init():
    for _ in range(num_displays):
        spi.xfer2([0x0C, 0x01])  # Shutdown: Normal Operation
        spi.xfer2([0x09, 0x00])  # Decode mode: No decode (raw segments)
        spi.xfer2([0x0B, 0x07])  # Scan limit: Display digits 0-7
        spi.xfer2([0x0A, 0x08])  # Intensity: Medium brightness
        spi.xfer2([0x0F, 0x00])  # Display test: Off

# Send data to all displays
def send_to_displays(digit, value):
    # Reverse the order of digit addressing
    for display in range(num_displays):
        # Send data in reverse order
        spi.xfer2([9 - digit, segment_map.get(value, 0x00)])

# Clear all displays
def clear_displays():
    for digit in range(1, 9):  # 8 digits
        for display in range(num_displays):
            send_to_displays(digit, 0x00)

max7219_init()
clear_displays()

# Test pattern: Display the numbers 0-7 across all digits
for digit in range(1, 9):  # 8 digits
    for display in range(num_displays):
        send_to_displays(digit, (digit - 1) % 8)  # Correctly show digits 0-7

time.sleep(5)  # Wait for 5 seconds to observe the pattern

clear_displays()  # Clear the displays after the pattern

# Shutdown the MAX7219
for _ in range(num_displays):
    spi.xfer2([0x0C, 0x00])  # Shutdown: Power down

spi.close()

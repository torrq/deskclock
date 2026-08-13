import spidev
import time

# Open SPI bus
spi = spidev.SpiDev()
spi.open(0, 0)  # (bus, device)
spi.max_speed_hz = 1000000

# Number of 8-digit displays
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
        spi.xfer2([0x0A, 0x01])  # Intensity: Low brightness
        spi.xfer2([0x0F, 0x00])  # Display test: Off

def display_test_pattern():
    # Display a pattern to all digits for testing
    for digit in range(1, 9):
        for value in range(10):
            send_to_displays(digit, value)
            time.sleep(0.5)  # Pause to observe the pattern

def send_to_displays(digit, value):
    # Send the same value to all displays for testing
    for display in range(num_displays):
        spi.xfer2([digit, segment_map.get(value, 0x00)])

max7219_init()
display_test_pattern()

import spidev
import time
from datetime import datetime

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
        spi.xfer2([0x0A, 0x01])  # Intensity: Low brightness
        spi.xfer2([0x0F, 0x00])  # Display test: Off

# Send data to all displays
def send_to_displays(digit, value):
    # Send the data to the correct digit in each display
    for display in range(num_displays):
        # Adjust the addressing here if necessary
        spi.xfer2([0x01 + (display * 8), segment_map.get(value, 0x00)])

# Clear all displays
def clear_displays():
    for digit in range(1, 9):  # 8 digits
        send_to_displays(digit, 0x00)

# Display the current time on the 7-segment displays
def display_time():
    while True:
        now = datetime.now()
        hour = now.hour
        minute = now.minute
        second = now.second
        ds = (now.microsecond // 100000) % 10
        print(f"Decisecond: {ds}")

        # Format the time as HHMMSS00
        time_str = f"{hour:02}{minute:02}{second:02}{ds:01}"

        if len(time_str) == 8:  # Ensure the time string has exactly 8 characters
            # Update displays: Display each digit of time_str
            for i in range(8):  # Use 8 because time_str always has 8 characters
                value = int(time_str[i])  # Extract individual digits
                send_to_displays(i + 1, value)  # i+1 to match display positions

        time.sleep(0.1)  # Update every 0.1 seconds

max7219_init()
clear_displays()
display_time()  # Start displaying the time

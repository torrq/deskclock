import spidev
import time
from datetime import datetime, timedelta

# Open SPI bus
spi = spidev.SpiDev()
spi.open(0, 0)  # (bus, device)
spi.max_speed_hz = 1000000

# Decimal point mask
DECIMAL_POINT_MASK = 0b10000000

# Updated 7-segment encoding for digits 0-9, dash, and other characters
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
    9: 0b01111011,  # 9
    10: 0b00000000, # Blank (X)
    11: 0b01100111, # P
    12: 0b00000001, # -- (dash)
    13: 0b01110111, # A
    14: 0b01111100, # b
    15: 0b00111001, # c
    16: 0b01011110, # d
    17: 0b01111001, # E
    18: 0b01110001, # F
    19: 0b01110110, # H
    20: 0b00111000, # L
    21: 0b01000000, # - (upper)
    22: 0b00001000, # _ (lower)
    23: 0b00000000  # Space
}

# Function to add decimal point
def add_decimal_point(value, show_dp):
    if show_dp:
        return value | DECIMAL_POINT_MASK
    return value

# Initialize the MAX7219
def max7219_init(num_displays=2):
    for addr in range(num_displays):
        spi.xfer2([0x0C | (addr << 8), 0x01])  # Shutdown: Normal Operation
        spi.xfer2([0x09 | (addr << 8), 0x00])  # Decode mode: No decode (raw segments)
        spi.xfer2([0x0B | (addr << 8), 0x07])  # Scan limit: Display digits 0-7
        spi.xfer2([0x0A | (addr << 8), 0x01])  # Intensity: Medium brightness
        spi.xfer2([0x0F | (addr << 8), 0x00])  # Display test: Off

# Send data to all displays in a single SPI transaction
def send_to_displays(display_data, last_display_data):
    num_displays = len(display_data)
    update_needed = False

    for digit in range(8):
        tx_data = []
        for display in range(num_displays - 1, -1, -1):  # Reverse to handle display addressing from your perspective
            data = display_data[display][digit] if digit < len(display_data[display]) else segment_map[10]  # Blank for out of range digits
            tx_data.append(8 - digit)  # Address (1-based index)
            tx_data.append(data)

            # Check if update is needed
            if last_display_data[display][digit] != data:
                update_needed = True

        if update_needed:
            spi.xfer2(tx_data)
            print(f"Display Data Changed: {display_data}")
            print(f"New Time: {datetime.now().strftime('%H:%M:%S')}")
            break  # Only print once per update

    return display_data

def format_time(hour, minute):
    # Determine AM or PM
    period = 11 if hour >= 12 else 13  # 'P' for PM, 'A' for AM

    # Convert to 12-hour format
    hour_12 = hour if 1 <= hour <= 12 else (hour - 12 if hour > 12 else 12)

    # Prepare digits and handle blanking for hours < 10
    if hour_12 < 10:
        hour_digits = [10, hour_12 % 10]  # Leading blank, then hour digit
    else:
        hour_digits = [hour_12 // 10, hour_12 % 10]  # Two digits for hour

    # Right-aligned: "X3-14XP"
    minute_tens = minute // 10
    minute_ones = minute % 10

    formatted = hour_digits + [12, minute_tens, minute_ones, 10, period]

    # Ensure all displays have a blank at the end
    formatted = formatted[:7] + [10]  # Cut off any extra and ensure the last digit is blank

    # Add decimal points to the 7th digit (index 6) of both displays
    formatted_with_decimals = [
        add_decimal_point(segment_map.get(char, 10), idx == 6)
        for idx, char in enumerate(formatted)
    ]

    return formatted_with_decimals

# Display the current time on the first display (bottom) and 3-hour-ahead time on the second (top)
def display_time():
    last_display_data = [
        [segment_map[10] for _ in range(8)],  # Initialize with blanks
        [segment_map[10] for _ in range(8)]   # Initialize with blanks
    ]

    while True:
        now = datetime.now()
        future_time = now + timedelta(hours=3)  # Add 3 hours for the second display

        # Get hours and minutes for current and future times
        hour = now.hour
        minute = now.minute

        future_hour = future_time.hour
        future_minute = future_time.minute

        # Format current time and future time
        display_1_digits = format_time(hour, minute)  # Logical Display 1 (bottom)
        display_2_digits = format_time(future_hour, future_minute)  # Logical Display 2 (top)

        # Prepare data for all displays
        all_display_data = [display_1_digits, display_2_digits]

        # Send data to all displays if different from last sent
        last_display_data = send_to_displays(all_display_data, last_display_data)

        time.sleep(1)  # Update every second

# Initialize and run the display
max7219_init(num_displays=2)
display_time()  # Start displaying the time

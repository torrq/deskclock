#!/usr/bin/python3
import spidev, signal, sys, time
from datetime import datetime

# Open SPI bus
spi = spidev.SpiDev()
spi.open(0, 0)  # (bus, device)
spi.max_speed_hz = 500000

# Number of 8-digit displays (each display consists of two 4-digit modules)
num_displays = 1

# 7-segment encoding for digits 0-9 and additional characters
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
    10: 0b00000000, # blank
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

# Global variable to store AM/PM
am_pm = "AM"

# Initialize the MAX7219
def max7219_init():
    for _ in range(num_displays):
        spi.xfer2([0x0C, 0x01])  # Shutdown: Normal Operation
        spi.xfer2([0x09, 0x00])  # Decode mode: No decode (raw segments)
        spi.xfer2([0x0B, 0x07])  # Scan limit: Display digits 0-7
        spi.xfer2([0x0A, 0x01])  # Intensity: Medium brightness
        spi.xfer2([0x0F, 0x00])  # Display test: Off

# Send data to all displays
def send_to_displays(digit, value):
    if digit == 2 and value == 0:   # if hour is in single digits, blank out the leading 0
        value = 10
    if digit in [4]:                # space between hour and minute
        value = 12
    if digit in [1,7,8]:            # spaces to blank out
        value = 10
    if digit == 8:
        if am_pm == "PM":           # AM or PM
            value = 11
        else:
            value = 13
    for display in range(num_displays):
        # Reverse the order of digit addressing
        spi.xfer2([9 - digit, segment_map.get(value, 0x00)])

# Clear all displays
def clear_displays():
    for digit in range(1, 9):  # 8 digits
        for display in range(num_displays):
            send_to_displays(digit, 0x00)

# Shutdown all displays
def shutdown_displays():
    for _ in range(num_displays):
        spi.xfer2([0x0C, 0x00])  # Shutdown: Enter shutdown mode

# Signal handler for graceful exit
def signal_handler(sig, frame):
    print("\nCtrl-C pressed. Quitting.")
    shutdown_displays()
    sys.exit(0)

# Display the current time on the 7-segment displays
def display_time():
    global am_pm  # Declare am_pm as global to modify it

    while True:
        now = datetime.now()
        hour_24 = now.hour
        minute = now.minute
        second = now.second

        # Determine AM or PM
        if hour_24 >= 12:
            am_pm = "PM"
        else:
            am_pm = "AM"        

        # Convert 24-hour format to 12-hour format
        if hour_24 == 0:
            hour_12 = 12
        elif hour_24 > 12:
            hour_12 = hour_24 - 12
        else:
            hour_12 = hour_24 
       
        # Format the time as HHMM
        time_str = f"0{hour_12:02}0{minute:02}00"
        
        if len(time_str) == 8:  # Ensure the time string has exactly 6 characters
            # Update displays: Display each digit of time_str
            for i in range(8):  # Use 6 because time_str always has 6 characters
                value = int(time_str[i])  # Extract individual digits
                send_to_displays(i + 1, value)  # i+1 to match display positions
        
        time.sleep(0.5)  # Update every second

# Set up the signal handler for Ctrl+C (SIGINT)
signal.signal(signal.SIGINT, signal_handler)

max7219_init()
clear_displays()
display_time()  # Start displaying the time

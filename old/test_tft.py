import time
import sys
import RPi.GPIO as GPIO

# Pins (BCM mode)
DC_PIN = 24  # Green wire
RST_PIN = 25 # Blue wire
CS_PIN = 22  # Purple wire (moved to Pin 15/GPIO 22)
MOSI_PIN = 10 # White wire (Pin 19)
CLK_PIN = 11  # Orange wire (Pin 23)

def setup():
    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(False)
    for pin in [DC_PIN, RST_PIN, CS_PIN, MOSI_PIN, CLK_PIN]:
        GPIO.setup(pin, GPIO.OUT)
    
    GPIO.output(CS_PIN, GPIO.HIGH)
    GPIO.output(CLK_PIN, GPIO.LOW)
    GPIO.output(MOSI_PIN, GPIO.LOW)

def reset():
    GPIO.output(RST_PIN, GPIO.HIGH)
    time.sleep(0.05)
    GPIO.output(RST_PIN, GPIO.LOW)
    time.sleep(0.05)
    GPIO.output(RST_PIN, GPIO.HIGH)
    time.sleep(0.1)

def bitbang_byte(b):
    for i in range(8):
        # MSB first
        bit = (b >> (7 - i)) & 1
        GPIO.output(MOSI_PIN, bit)
        # Pulse clock
        GPIO.output(CLK_PIN, GPIO.HIGH)
        GPIO.output(CLK_PIN, GPIO.LOW)

def send_command(cmd):
    GPIO.output(DC_PIN, GPIO.LOW)
    GPIO.output(CS_PIN, GPIO.LOW)
    bitbang_byte(cmd)
    GPIO.output(CS_PIN, GPIO.HIGH)

def send_data(data_bytes):
    GPIO.output(DC_PIN, GPIO.HIGH)
    GPIO.output(CS_PIN, GPIO.LOW)
    for b in data_bytes:
        bitbang_byte(b)
    GPIO.output(CS_PIN, GPIO.HIGH)

def init_display():
    print("Initializing...")
    send_command(0x01) # SWRESET
    time.sleep(0.15)
    send_command(0x11) # SLPOUT
    time.sleep(0.15)
    
    send_command(0x36) # MADCTL
    send_data([0x00])  # RGB, standard orientation
    
    send_command(0x3A) # COLMOD
    send_data([0x05])  # 16-bit
    
    send_command(0x29) # DISPON
    time.sleep(0.1)

def draw_test_square():
    print("Drawing 40x40 cyan square...")
    # Set window: X=20 to 59, Y=20 to 59 (40x40 pixels)
    send_command(0x2A)
    send_data([0x00, 20, 0x00, 59])
    
    send_command(0x2B)
    send_data([0x00, 20, 0x00, 59])
    
    send_command(0x2C) # RAMWR
    
    GPIO.output(DC_PIN, GPIO.HIGH)
    GPIO.output(CS_PIN, GPIO.LOW)
    
    # 40x40 = 1600 pixels. Cyan = [0x07, 0xFF]
    for _ in range(1600):
        bitbang_byte(0x07)
        bitbang_byte(0xFF)
        
    GPIO.output(CS_PIN, GPIO.HIGH)
    print("Done!")

if __name__ == "__main__":
    setup()
    reset()
    init_display()
    draw_test_square()

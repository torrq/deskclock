#!/usr/bin/env python3
import spidev
import time
import signal
import sys
import threading
import json
import urllib.request
import socket
import spidev
from datetime import datetime, timedelta, timezone
try:
    import RPi.GPIO as GPIO
except ImportError:
    print("Warning: RPi.GPIO not found. TFT will not work without it.")

# 7-segment encoding for digits 0-9 and some characters
SEGMENT_MAP = {
    '0': 0b01111110,
    '1': 0b00110000,
    '2': 0b01101101,
    '3': 0b01111001,
    '4': 0b00110011,
    '5': 0b01011011,
    '6': 0b01011111,
    '7': 0b01110000,
    '8': 0b01111111,
    '9': 0b01111011,
    ' ': 0b00000000,
    'A': 0b01110111,
    'P': 0b01100111,
    '-': 0b00000001,
}

# 5x7 Pixel Font for TFT
FONT_5x7 = {
    # Digits
    '0': [0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E],
    '1': [0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E],
    '2': [0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F],
    '3': [0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E],
    '4': [0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02],
    '5': [0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E],
    '6': [0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E],
    '7': [0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08],
    '8': [0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E],
    '9': [0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C],
    # Symbols
    ':': [0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00],
    ' ': [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    '%': [0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03],
    '-': [0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00],
    # Alphabet
    'A': [0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11],
    'B': [0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E],
    'C': [0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E],
    'D': [0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E],
    'E': [0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F],
    'F': [0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10],
    'G': [0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F],
    'H': [0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11],
    'I': [0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E],
    'J': [0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C],
    'K': [0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11],
    'L': [0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F],
    'M': [0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11],
    'N': [0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11],
    'O': [0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E],
    'P': [0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10],
    'Q': [0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D],
    'R': [0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11],
    'S': [0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E],
    'T': [0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04],
    'U': [0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E],
    'V': [0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04],
    'W': [0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11],
    'X': [0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11],
    'Y': [0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04],
    'Z': [0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F],
}

# MAX7219 Register map
REG_NOOP = 0x00
REG_DIGIT_0 = 0x01 # Registers 0x01 to 0x08 are for the 8 digits
REG_DECODE = 0x09
REG_INTENSITY = 0x0A
REG_SCAN_LIMIT = 0x0B
REG_SHUTDOWN = 0x0C
REG_DISPLAY_TEST = 0x0F

class MAX7219Chain:
    """
    Controls a daisy-chain of MAX7219 LED matrix modules using Software SPI (Bitbanging).
    This completely isolates the 5V MAX7219 signals from the 3.3V TFT hardware SPI bus.
    """
    def __init__(self, din_pin=17, clk_pin=18, cs_pin=23, num_displays=2):
        self.num_displays = num_displays
        self.din_pin = din_pin
        self.clk_pin = clk_pin
        self.cs_pin = cs_pin
        
        GPIO.setmode(GPIO.BCM)
        GPIO.setwarnings(False)
        GPIO.setup(self.din_pin, GPIO.OUT)
        GPIO.setup(self.clk_pin, GPIO.OUT)
        GPIO.setup(self.cs_pin, GPIO.OUT)
        
        GPIO.output(self.cs_pin, GPIO.HIGH)
        GPIO.output(self.clk_pin, GPIO.LOW)
        
        self.init_displays()

    def _shift_out(self, byte_data):
        for i in range(8):
            bit = (byte_data >> (7 - i)) & 1
            GPIO.output(self.din_pin, bit)
            GPIO.output(self.clk_pin, GPIO.HIGH)
            GPIO.output(self.clk_pin, GPIO.LOW)

    def write_cmd(self, register, data):
        GPIO.output(self.cs_pin, GPIO.LOW)
        for _ in range(self.num_displays):
            self._shift_out(register)
            self._shift_out(data)
        GPIO.output(self.cs_pin, GPIO.HIGH)

    def write_cmd_chain(self, register, data_list):
        """Send specific data to each module."""
        GPIO.output(self.cs_pin, GPIO.LOW)
        # Data is shifted through, so last module gets data first
        for i in range(self.num_displays - 1, -1, -1):
            self._shift_out(register)
            self._shift_out(data_list[i])
        GPIO.output(self.cs_pin, GPIO.HIGH)

    def send_command_all(self, register, value):
        self.write_cmd(register, value)

    def init_displays(self):
        self.send_command_all(REG_SHUTDOWN, 0x01)     # Wake up from shutdown
        self.send_command_all(REG_DECODE, 0x00)       # No decode, raw segments
        self.send_command_all(REG_SCAN_LIMIT, 0x07)   # Display all 8 digits
        self.send_command_all(REG_INTENSITY, 0x04)    # Medium intensity (0x00 to 0x0F)
        self.send_command_all(REG_DISPLAY_TEST, 0x00) # Disable display test

    def shutdown(self):
        self.clear()
        self.send_command_all(REG_SHUTDOWN, 0x00)

    def clear(self):
        for digit_reg in range(1, 9):
            self.write_cmd(digit_reg, 0x00)

class ST7735:
    """
    Driver for ST7735 TFT display via Native Hardware SPI (spidev).
    The Linux kernel natively handles CS via the dtoverlay=spi0-1cs,cs0_pin=22 boot config.
    """
    def __init__(self, spi_bus=0, spi_device=0, dc_pin=24, rst_pin=25):
        self.dc_pin = dc_pin
        self.rst_pin = rst_pin
        
        if 'RPi.GPIO' in sys.modules:
            GPIO.setmode(GPIO.BCM)
            GPIO.setwarnings(False)
            GPIO.setup(self.dc_pin, GPIO.OUT)
            GPIO.setup(self.rst_pin, GPIO.OUT)
            
            self.spi = spidev.SpiDev()
            self.spi.open(spi_bus, spi_device)
            # 4MHz is safe for jumper wires
            self.spi.max_speed_hz = 4000000 
            self.spi.mode = 0
            
            print("Initializing TFT via Native Hardware SPI (spidev)...")
            self.reset()
            self.init_display()
        else:
            print("TFT initialization skipped because RPi.GPIO is missing!")

    def reset(self):
        GPIO.output(self.rst_pin, GPIO.HIGH)
        time.sleep(0.05)
        GPIO.output(self.rst_pin, GPIO.LOW)
        time.sleep(0.05)
        GPIO.output(self.rst_pin, GPIO.HIGH)
        time.sleep(0.1)

    def send_command(self, cmd):
        GPIO.output(self.dc_pin, GPIO.LOW)
        self.spi.xfer2([cmd])

    def send_data(self, data):
        GPIO.output(self.dc_pin, GPIO.HIGH)
        self.spi.xfer2(data)

    def init_display(self):
        self.send_command(0x01) # SWRESET
        time.sleep(0.15)
        self.send_command(0x11) # SLPOUT
        time.sleep(0.15)
        
        # 0x60 = MX=1, MV=1 (Landscape mode, Row/Col exchange)
        self.send_command(0x36) # MADCTL
        self.send_data([0x60])  # Landscape orientation
        
        self.send_command(0x3A) # COLMOD
        self.send_data([0x05])  # 16-bit
        
        self.send_command(0x29) # DISPON
        time.sleep(0.1)

    def fill_color(self, hex_color):
        if 'RPi.GPIO' not in sys.modules:
            return
            
        color_bytes = [(hex_color >> 8) & 0xFF, hex_color & 0xFF]
        
        # Set Full Screen Bounding Box for Landscape (160x128)
        self.send_command(0x2A)
        self.send_data([0x00, 0x00, 0x00, 0x9F]) # X: 0 to 159
        self.send_command(0x2B)
        self.send_data([0x00, 0x00, 0x00, 0x7F]) # Y: 0 to 127
        
        self.send_command(0x2C) # RAMWR
        
        GPIO.output(self.dc_pin, GPIO.HIGH)
        
        print("Clearing background via Native Hardware SPI (Instant)...")
        chunk = color_bytes * 2048
        for _ in range(10):
            self.spi.xfer2(chunk)

    def draw_text(self, start_x, start_y, text, fg_color, bg_color, scale=2, clear_to_edges=False):
        if 'RPi.GPIO' not in sys.modules:
            return
            
        height = 7 * scale
        char_width = 5
        
        pixels = []
        fg_high, fg_low = (fg_color >> 8) & 0xFF, fg_color & 0xFF
        bg_high, bg_low = (bg_color >> 8) & 0xFF, bg_color & 0xFF
        
        text_width = len(text) * (char_width + 1) * scale
        
        if clear_to_edges:
            left_margin = start_x
            right_margin = 160 - (start_x + text_width)
            if right_margin < 0: right_margin = 0
            
            # Send bounding box for FULL WIDTH of the screen
            self.send_command(0x2A)
            self.send_data([0x00, 0, 0x00, 159])
            self.send_command(0x2B)
            self.send_data([0x00, start_y, 0x00, start_y + height - 1])
        else:
            # Send bounding box for JUST the text
            self.send_command(0x2A)
            self.send_data([0x00, start_x, 0x00, start_x + text_width - 1])
            self.send_command(0x2B)
            self.send_data([0x00, start_y, 0x00, start_y + height - 1])
            
        # Build pixel buffer row by row
        for row in range(height):
            font_row = row // scale
            
            if clear_to_edges:
                for _ in range(left_margin):
                    pixels.extend([bg_high, bg_low])
                    
            for char in text:
                pattern = FONT_5x7.get(char, FONT_5x7[' '])[font_row]
                for col in range(char_width):
                    bit = (pattern >> (4 - col)) & 1
                    for _ in range(scale):
                        if bit:
                            pixels.extend([fg_high, fg_low])
                        else:
                            pixels.extend([bg_high, bg_low])
                # Inter-character spacing
                for _ in range(scale):
                    pixels.extend([bg_high, bg_low])
                    
            if clear_to_edges:
                for _ in range(right_margin):
                    pixels.extend([bg_high, bg_low])
                    
        self.send_command(0x2C)
        GPIO.output(self.dc_pin, GPIO.HIGH)
        
        # spidev has a 4096 byte limit. Chunk our pixel array.
        for i in range(0, len(pixels), 4096):
            self.spi.xfer2(pixels[i:i+4096])

    def draw_text_centered(self, y, text, fg_color, bg_color, scale=2):
        # Calculate EXACT pixel width of the raw text
        char_width = 5
        width_pixels = len(text) * (char_width + 1) * scale
        
        # Pixel-perfect centering
        start_x = max(0, (160 - width_pixels) // 2)
        
        # Draw and clear everything else on that row to black
        self.draw_text(start_x, y, text, fg_color, bg_color, scale, clear_to_edges=True)

    def shutdown(self):
        try:
            self.spi.close()
        except AttributeError:
            pass

class Clock:
    def __init__(self, chain, tft):
        self.chain = chain
        self.tft = tft
        self.running = True
        
        self.weather_data = {
            "temp": "--C", 
            "desc": "FETCHING...", 
            "hum": "--%", 
            "updated": True
        }
        
        # Start background thread to fetch weather so the clocks never freeze
        self.weather_thread = threading.Thread(target=self.weather_loop, daemon=True)
        self.weather_thread.start()

    def weather_loop(self):
        # Enforce strict socket timeouts to prevent infinite hanging
        socket.setdefaulttimeout(10)
        
        while self.running:
            try:
                print("Fetching live weather data...", flush=True)
                # Use HTTP instead of HTTPS to avoid SSL overhead/clock issues on the Pi
                req = urllib.request.Request("http://wttr.in/Vancouver?format=j1", headers={'User-Agent': 'curl/7.68.0'})
                
                with urllib.request.urlopen(req, timeout=10) as response:
                    raw_data = response.read().decode()
                    data = json.loads(raw_data)
                    
                    cc = data['current_condition'][0]
                    self.weather_data['temp'] = f"{cc['temp_C']}C"
                    
                    # Clean description for our font
                    desc = cc['weatherDesc'][0]['value'].upper()
                    # Replace anything not in our font with a space
                    clean_desc = "".join([c if c in FONT_5x7 else " " for c in desc])
                    self.weather_data['desc'] = clean_desc
                    
                    self.weather_data['hum'] = f"HUM {cc['humidity']}%"
                    self.weather_data['updated'] = True
                    print("Weather updated successfully!", flush=True)
            except Exception as e:
                print(f"Weather fetch error: {type(e).__name__} - {e}", flush=True)
            
            # Wait 15 minutes between fetches
            for _ in range(15 * 60):
                if not self.running: break
                time.sleep(1)

    def format_time_digits(self, dt):
        """
        Format a datetime into a list of 8 raw bytes for the 7-segment displays.
        Format layout: " HH MM  P" (e.g., " 12 45  P")
        """
        hour_24 = dt.hour
        minute = dt.minute

        am_pm = 'P' if hour_24 >= 12 else 'A'
        
        hour_12 = hour_24 % 12
        if hour_12 == 0:
            hour_12 = 12
            
        # Create an 8-character string, space-padded if the hour is a single digit
        time_str = f"{hour_12:2d} {minute:02d}  {am_pm}"
        time_str = time_str.ljust(8, ' ')[:8]
        
        digits = []
        for char in time_str:
            digits.append(SEGMENT_MAP.get(char, 0x00))
            
        return digits

    def display_time(self):
        """
        Main loop to constantly update the displays.
        """
        print("Clock started! Press Ctrl+C to exit.")
        manila_tz = timezone(timedelta(hours=8))
        
        last_utc_minute = -1
        
        while self.running:
            now = datetime.now()
            utc_now = datetime.now(timezone.utc)
            manila_time = utc_now.astimezone(manila_tz)

            # Update LED Matrices
            digits_top = self.format_time_digits(now)
            digits_bot = self.format_time_digits(manila_time)

            for i in range(8):
                register = 8 - i
                self.chain.write_cmd_chain(register, [digits_top[i], digits_bot[i]])
                
            # Update TFT Weather
            if self.weather_data.get('updated'):
                self.tft.draw_text_centered(10, "VANCOUVER", 0xFFE0, 0x0000, scale=2)
                self.tft.draw_text_centered(40, self.weather_data['temp'], 0x07FF, 0x0000, scale=4)
                self.tft.draw_text_centered(80, self.weather_data['desc'], 0xFFFF, 0x0000, scale=2)
                self.tft.draw_text_centered(105, self.weather_data['hum'], 0x07E0, 0x0000, scale=2)
                self.weather_data['updated'] = False
                
            # Sleep until the top of the next second
            next_sec = 1.0 - (time.time() % 1.0)
            time.sleep(next_sec)

def main():
    # Setup MAX7219 on pure software SPI (Bitbang) to prevent hardware conflict
    # DIN = GPIO 17 (Pin 11), CLK = GPIO 18 (Pin 12), CS = GPIO 23 (Pin 16)
    chain = MAX7219Chain(din_pin=17, clk_pin=18, cs_pin=23, num_displays=2)
    
    # Setup TFT on native hardware SPI (spidev)
    # The kernel natively toggles CS via GPIO 22 based on our /boot/config.txt overlay.
    tft = ST7735(spi_bus=0, spi_device=0, dc_pin=24, rst_pin=25)
    
    # Fill the TFT with a sleek black background
    print("Clearing TFT screen...")
    tft.fill_color(0x0000)
    
    # Initialize Clock with both displays
    clock = Clock(chain, tft)

    def signal_handler(sig, frame):
        print("\nExiting and clearing displays...")
        clock.running = False
        chain.shutdown()
        tft.shutdown()
        if 'RPi.GPIO' in sys.modules:
            GPIO.cleanup()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    try:
        clock.display_time()
    except Exception as e:
        print(f"Error: {e}")
        chain.shutdown()
        tft.shutdown()
        if 'RPi.GPIO' in sys.modules:
            GPIO.cleanup()

if __name__ == "__main__":
    main()

import spidev
import RPi.GPIO as GPIO
from time import sleep

# MAX7219 commands
OP_NOOP = 0x00
OP_DIGIT0 = 0x01
OP_DIGIT1 = 0x02
OP_DIGIT2 = 0x03
OP_DIGIT3 = 0x04
OP_DIGIT4 = 0x05
OP_DIGIT5 = 0x06
OP_DIGIT6 = 0x07
OP_DIGIT7 = 0x08
OP_DECODEMODE = 0x09
OP_INTENSITY = 0x01
OP_SCANLIMIT = 0x0B
OP_SHUTDOWN = 0x0C
OP_DISPLAYTEST = 0x0F

class LedControl:
    def __init__(self, data_pin, clk_pin, cs_pin, num_devices):
        self.data_pin = data_pin
        self.clk_pin = clk_pin
        self.cs_pin = cs_pin
        self.num_devices = min(max(num_devices, 1), 8)
        self.spi = spidev.SpiDev()
        self.spi.open(0, 0)  # SPI bus 0, device 0
        self.spi.max_speed_hz = 100000  # Adjust if needed
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(cs_pin, GPIO.OUT)
        GPIO.output(cs_pin, GPIO.HIGH)
        self.initialize()

    def spi_transfer(self, addr, opcode, data):
        spidata = [0] * (self.num_devices * 2)
        spidata[addr * 2] = opcode
        spidata[addr * 2 + 1] = data
        GPIO.output(self.cs_pin, GPIO.LOW)
        self.spi.xfer2(spidata)
        GPIO.output(self.cs_pin, GPIO.HIGH)

    def initialize(self):
        for i in range(self.num_devices):
            self.spi_transfer(i, OP_DISPLAYTEST, 0)
            self.set_scan_limit(i, 7)
            self.spi_transfer(i, OP_DECODEMODE, 0)
            self.clear_display(i)
            self.shutdown(i, True)

    def shutdown(self, addr, b):
        self.spi_transfer(addr, OP_SHUTDOWN, 0 if b else 1)

    def set_scan_limit(self, addr, limit):
        if 0 <= limit < 8:
            self.spi_transfer(addr, OP_SCANLIMIT, limit)

    def set_intensity(self, addr, intensity):
        if 0 <= intensity < 16:
            self.spi_transfer(addr, OP_INTENSITY, intensity)

    def clear_display(self, addr):
        for i in range(8):
            self.spi_transfer(addr, i + 1, 0x00)

    def set_digit(self, addr, digit, value, dp=False):
        if 0 <= digit < 8 and 0 <= value < 16:
            v = value  # Map value to segment encoding if needed
            if dp:
                v |= 0x80  # Add decimal point
            self.spi_transfer(addr, digit + 1, v)

def main():
    data_pin = 19  # Example GPIO pin for MOSI
    clk_pin = 23    # Example GPIO pin for CLK
    cs_pin = 24     # Example GPIO pin for CS
    num_devices = 4  # Adjust based on your setup
    lc = LedControl(data_pin, clk_pin, cs_pin, num_devices)
    
    lc.clear_display(0)  # Clear display 0
    for i in range(20):
        lc.set_digit(0, 0, i)
        sleep(1)
        lc.clear_display(0)

if __name__ == "__main__":
    main()


import spidev
from time import sleep

# Define MAX7219 commands
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
OP_INTENSITY = 0x0A
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
        self.spi.open(0, 0)  # Bus 0, Device 0
        self.spi.max_speed_hz = 1000000  # Adjust speed if necessary
        self.initialize()

    def spi_transfer(self, addr, opcode, data):
        # Create a byte array for the data to shift out
        spidata = [0] * (self.num_devices * 2)
        spidata[addr * 2] = opcode
        spidata[addr * 2 + 1] = data
        self.spi.xfer2(spidata)

    def initialize(self):
        for i in range(self.num_devices):
            self.spi_transfer(i, OP_DISPLAYTEST, 0)
            self.set_scan_limit(i, 7)
            self.spi_transfer(i, OP_DECODEMODE, 0)
            self.clear_display(i)
            self.shutdown(i, True)

    def shutdown(self, addr, b):
        if b:
            self.spi_transfer(addr, OP_SHUTDOWN, 0)
        else:
            self.spi_transfer(addr, OP_SHUTDOWN, 1)

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
            v = value  # Map value to segment encoding here if needed
            if dp:
                v |= 0x80  # Add decimal point
            self.spi_transfer(addr, digit + 1, v)

    def set_char(self, addr, digit, value, dp=False):
        if 0 <= digit < 8:
            # Map character to segment encoding if needed
            v = ord(value)  # Simple ASCII mapping
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
    for i in range(10):
        lc.set_digit(0, 0, i)
        sleep(1)
        lc.clear_display(0)

if __name__ == "__main__":
    main()

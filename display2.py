import spidev
import time

# Define the SPI parameters
SPI_BUS = 0
SPI_DEVICE = 0
SPI_SPEED = 500000
SPI_MODE = 0

# Define the MAX7219 register addresses
REG_DECODEMODE = 0x09
REG_INTENSITY = 0x0A
REG_SCANLIMIT = 0x0B
REG_SHUTDOWN = 0x0C
REG_DISPLAYTEST = 0x0F
REG_DIGIT = 0x01

# Segment map for digits and special characters
charTable = [
    0b01111110, 0b00110000, 0b01101101, 0b01111001, 0b00110011, 0b01011011, 0b01011111, 0b01110000,
    0b01111111, 0b01111011, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b10000000, 0b00000001, 0b10000000, 0b00000000,
    0b01111110, 0b00110000, 0b01101101, 0b01111001, 0b00110011, 0b01011011, 0b01011111, 0b01110000,
    0b01111111, 0b01111011, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b01110111, 0b00011111, 0b00001101, 0b00111101, 0b01001111, 0b01000111, 0b00000000,
    0b00110111, 0b00000000, 0b00000000, 0b00000000, 0b00001110, 0b00000000, 0b00000000, 0b00000000,
    0b01100111, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00001000,
    0b00000000, 0b01110111, 0b00011111, 0b00001101, 0b00111101, 0b01001111, 0b01000111, 0b00000000,
    0b00110111, 0b00000000, 0b00000000, 0b00000000, 0b00001110, 0b00000000, 0b00010101, 0b00011101,
    0b01100111, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000
]

# Initialize SPI
spi = spidev.SpiDev()
spi.open(SPI_BUS, SPI_DEVICE)
spi.max_speed_hz = SPI_SPEED
spi.mode = SPI_MODE

def spi_transfer(addr, opcode, data):
    """Send command to the MAX7219 device."""
    max_devices = 4
    spidata = [0] * (max_devices * 2)
    offset = addr * 2
    spidata[offset + 1] = opcode
    spidata[offset] = data

    spi.xfer2(spidata)

def initialize_displays():
    """Initialize MAX7219 displays."""
    for i in range(4):
        spi_transfer(i, REG_DISPLAYTEST, 0x00)   # Turn off display test
        spi_transfer(i, REG_SHUTDOWN, 0x01)      # Wake up the display
        spi_transfer(i, REG_SCANLIMIT, 0x07)     # Display digits 0-7
        spi_transfer(i, REG_DECODEMODE, 0x00)    # Use LED matrix
        spi_transfer(i, REG_INTENSITY, 0x01)     # Set intensity to minimum

def clear_displays():
    """Clear all digits on all displays."""
    for i in range(1, 9):  # Digits 1 to 8
        for d in range(4):
            spi_transfer(d, REG_DIGIT + i, 0x00)  # blank

def display_number(number, display_index):
    """Display a number on a specific display (display_index 0 to 3)."""
    for i in range(1, 9):  # Digits 1 to 8
        if i == 8:  # Update only the last digit
            spi_transfer(display_index, REG_DIGIT + i, charTable[number])
        else:
            spi_transfer(display_index, REG_DIGIT + i, 0x00)  # blank

def test_displays():
    """Test all displays with different numbers."""
    initialize_displays()
    time.sleep(1)

    numbers = [1, 2, 3, 4]  # Different numbers for each display

    for i in range(4):
        clear_displays()
        display_number(numbers[i], i)
        time.sleep(2)

    clear_displays()

if __name__ == "__main__":
    test_displays()


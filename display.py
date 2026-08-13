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

# 7-segment encoding for digits 0-9 and special characters
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
    10: 0b00000000,  # blank
    11: 0b01100111,  # P
    12: 0b00000001   # --
}

# Initialize SPI
spi = spidev.SpiDev()
spi.open(SPI_BUS, SPI_DEVICE)
spi.max_speed_hz = SPI_SPEED
spi.mode = SPI_MODE

def send_command(register, value):
    """Send a command to the MAX7219 display."""
    spi.xfer2([register, value])

def initialize_displays():
    """Initialize all MAX7219 displays."""
    send_command(REG_DISPLAYTEST, 0x00)   # Turn off display test mode
    send_command(REG_SHUTDOWN, 0x01)      # Wake up the display
    send_command(REG_SCANLIMIT, 0x07)     # Display digits 0-7
    send_command(REG_DECODEMODE, 0x00)    # Use LED matrix (no BCD decoding)
    send_command(REG_INTENSITY, 0x01)     # Set brightness to minimum

def clear_displays():
    """Clear all digits on all displays."""
    for i in range(1, 9):  # Digits 1 to 8
        send_command(REG_DIGIT + i, segment_map[10])  # blank

def display_number(number, display_index):
    """Display a number on a specific display (display_index 1 to 4)."""
    for digit_pos in range(1, 9):  # Digits 1 to 8
        # Ensure only the last digit is updated for the specified display
        if digit_pos == 8:
            send_command(REG_DIGIT + digit_pos, segment_map[number])
        else:
            send_command(REG_DIGIT + digit_pos, segment_map[10])  # blank

def test_displays():
    """Test all displays with different numbers."""
    initialize_displays()
    time.sleep(1)

    numbers = [1, 2, 3, 4]  # Different numbers to display on each of the 4 displays

    for i in range(4):
        clear_displays()
        # Only update the specified display
        for display in range(1, 5):
            if display == i + 1:
                display_number(numbers[i], display)
        time.sleep(2)

    clear_displays()

if __name__ == "__main__":
    test_displays()

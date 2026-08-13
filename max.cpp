#include <iostream>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <unistd.h>

// SPI device file
const char *spi_device = "/dev/spidev0.0";

// SPI parameters
const int spi_speed = 500000;  // SPI speed (500 kHz)
const int spi_mode = 0;        // SPI mode 0

void spi_transfer(int fd, uint8_t opcode, uint8_t data) {
    uint8_t tx_buf[2] = {opcode, data};
    uint8_t rx_buf[2] = {0};
    wiringPiSPIDataRW(0, tx_buf, 2); // SPI transfer
}

void initialize_max7219(int fd) {
    // Example initialization sequence for MAX7219
    spi_transfer(fd, 0x0C, 0x01); // Shutdown register: normal operation
    spi_transfer(fd, 0x0F, 0x00); // Display test register: no test
    spi_transfer(fd, 0x0B, 0x07); // Scan limit register: 8 digits
    spi_transfer(fd, 0x0A, 0x0F); // Intensity register: max intensity
    spi_transfer(fd, 0x09, 0x00); // Decode mode register: no decode
    clear_display(fd);            // Clear display
}

void clear_display(int fd) {
    for (int i = 1; i <= 8; ++i) {
        spi_transfer(fd, i, 0x00); // Set all digits to 0
    }
}

void set_digit(int fd, int digit, uint8_t value) {
    if (digit < 1 || digit > 8) return;
    spi_transfer(fd, digit, value);
}

int main() {
    if (wiringPiSetup() == -1) {
        std::cerr << "Failed to setup wiringPi" << std::endl;
        return 1;
    }

    int spi_fd = wiringPiSPISetup(0, spi_speed);
    if (spi_fd == -1) {
        std::cerr << "Failed to setup SPI" << std::endl;
        return 1;
    }

    initialize_max7219(spi_fd);

    // Example: display '1' on digit 1
    set_digit(spi_fd, 1, 0x06); // '1' in 7-segment encoding

    sleep(5);  // Keep the display on for 5 seconds

    clear_display(spi_fd); // Clear display before exiting

    close(spi_fd); // Close SPI device

    return 0;
}


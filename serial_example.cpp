#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <cstring>

class LedControl {
private:
    int spi_fd;
    uint8_t spi_mode;
    uint8_t spi_bits_per_word;
    uint32_t spi_speed;

    void spiTransfer(uint8_t opcode, uint8_t data) {
        uint8_t buffer[2];
        buffer[0] = opcode;
        buffer[1] = data;
        struct spi_ioc_transfer transfer;
        memset(&transfer, 0, sizeof(transfer));
        transfer.tx_buf = (unsigned long)buffer;
        transfer.len = sizeof(buffer);
        transfer.speed_hz = spi_speed;
        transfer.bits_per_word = spi_bits_per_word;
        ioctl(spi_fd, SPI_IOC_MESSAGE(1), &transfer);
    }

public:
    LedControl(const char* spi_device) {
        spi_fd = open(spi_device, O_RDWR);
        if (spi_fd < 0) {
            perror("Failed to open SPI device");
            exit(1);
        }
        spi_mode = SPI_MODE_0;
        spi_bits_per_word = 8;
        spi_speed = 500000;
        ioctl(spi_fd, SPI_IOC_WR_MODE, &spi_mode);
        ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits_per_word);
        ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
    }

    ~LedControl() {
        close(spi_fd);
    }

    void sendCommand(uint8_t opcode, uint8_t data) {
        spiTransfer(opcode, data);
    }

    void resetDisplay() {
        sendCommand(0x0C, 0x01); // Shutdown register to 0x01
        sendCommand(0x0B, 0x07); // Scan limit register to 0x07
        sendCommand(0x09, 0x00); // Decode mode register to 0x00
        sendCommand(0x0A, 0x0F); // Intensity register to 0x0F
        sendCommand(0x0F, 0x00); // Display test register to 0x00
    }

    void displayNumber(int number) {
        for (int i = 0; i < 8; ++i) {
            sendCommand(i + 1, number % 10);
            number /= 10;
        }
    }
};

int main() {
    LedControl lc("/dev/spidev0.1");
    lc.resetDisplay();
    lc.displayNumber(12345678);
    sleep(0.5);
    return 0;
}

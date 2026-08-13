import time
import max7219.led as led
from max7219.font import proportional, CP437_FONT

# Initialize the display (for 8 cascaded 4-digit modules)
device = led.sevensegment(cascaded=8)

# Clear the display
device.clear()

# Display a simple test pattern
for i in range(8):
    device.write_number(deviceId=i, value=i+1, dot=False)

time.sleep(5)

# Scroll a message across the displays
device.show_message("HELLO WORLD", font=proportional(CP437_FONT))


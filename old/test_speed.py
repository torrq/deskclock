import time
import math

start = time.time()
buffer = bytearray(160 * 128 * 2)

# Fill background (blue)
bg_color = [0x00, 0x1F] * (160 * 128)
buffer[:] = bytes(bg_color)

# Draw some text (simulate compositing overhead)
# (just doing a few loops to simulate writing pixels)
for i in range(1000):
    idx = i * 2
    buffer[idx] = 0xFF
    buffer[idx+1] = 0xFF

end = time.time()
print(f"Time to composite 1 frame: {(end - start)*1000:.2f} ms")

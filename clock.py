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
import random
import math
from datetime import datetime, timedelta, timezone

def interpolate_color(c1, c2, ratio):
    r1, g1, b1 = (c1 >> 11) & 0x1F, (c1 >> 5) & 0x3F, c1 & 0x1F
    r2, g2, b2 = (c2 >> 11) & 0x1F, (c2 >> 5) & 0x3F, c2 & 0x1F
    r = int(r1 + (r2 - r1) * ratio)
    g = int(g1 + (g2 - g1) * ratio)
    b = int(b1 + (b2 - b1) * ratio)
    return (r << 11) | (g << 5) | b
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
    ':': [0x00, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x00],
    '°': [0x0E, 0x0A, 0x0E, 0x00, 0x00, 0x00, 0x00],
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
FONT_32x48 = {
    '0': [
        0x00000000,
        0x00000000,
        0x00000000,
        0x001FE000,
        0x007FF800,
        0x01FFFC00,
        0x03FFFF00,
        0x03FFFF00,
        0x07FFFF80,
        0x0FFFFFC0,
        0x0FF87FC0,
        0x0FF03FC0,
        0x1FE01FE0,
        0x1FE01FE0,
        0x1FE01FE0,
        0x1FC00FE0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x1FC00FE0,
        0x1FE01FE0,
        0x1FE01FE0,
        0x1FE01FE0,
        0x1FF03FC0,
        0x0FF87FC0,
        0x0FFFFFC0,
        0x07FFFF80,
        0x03FFFF00,
        0x03FFFF00,
        0x00FFFC00,
        0x007FF800,
        0x001FE000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
    ],
    '1': [
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x0001FC00,
        0x0001FC00,
        0x0003FC00,
        0x0003FC00,
        0x0007FC00,
        0x000FFC00,
        0x003FFC00,
        0x007FFC00,
        0x01FFFC00,
        0x07FFFC00,
        0x0FFFFC00,
        0x0FFFFC00,
        0x0FFBFC00,
        0x0FF3FC00,
        0x0FC3FC00,
        0x0F83FC00,
        0x0E03FC00,
        0x0803FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x0003FC00,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
    ],
    '2': [
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x000FF000,
        0x007FFE00,
        0x00FFFF00,
        0x03FFFF80,
        0x03FFFFC0,
        0x07FFFFE0,
        0x0FFFFFE0,
        0x0FF83FF0,
        0x0FF01FF0,
        0x1FF00FF0,
        0x1FE00FF0,
        0x1FE00FF0,
        0x07E00FF0,
        0x00000FF0,
        0x00000FE0,
        0x00001FE0,
        0x00001FE0,
        0x00003FC0,
        0x00007FC0,
        0x0000FF80,
        0x0001FF00,
        0x0003FF00,
        0x0007FE00,
        0x000FFC00,
        0x001FF000,
        0x003FE000,
        0x007FC000,
        0x00FF8000,
        0x01FF0000,
        0x01FE0000,
        0x03FC0000,
        0x07FC0000,
        0x0FFFFFF0,
        0x0FFFFFF0,
        0x1FFFFFF0,
        0x1FFFFFF0,
        0x1FFFFFF0,
        0x3FFFFFF0,
        0x3FFFFFF0,
        0x3FFFFFF0,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
    ],
    '3': [
        0x00000000,
        0x00000000,
        0x00000000,
        0x001FC000,
        0x007FF800,
        0x01FFFC00,
        0x03FFFE00,
        0x07FFFF00,
        0x07FFFF80,
        0x0FFFFF80,
        0x0FF8FFC0,
        0x0FF07FC0,
        0x1FF03FC0,
        0x1FE03FC0,
        0x01E03FC0,
        0x00003FC0,
        0x00003F80,
        0x00007F80,
        0x0000FF00,
        0x0003FE00,
        0x0007FC00,
        0x0007F800,
        0x0007FC00,
        0x0007FF00,
        0x0007FF80,
        0x00003FC0,
        0x00001FE0,
        0x00001FE0,
        0x00000FF0,
        0x00000FF0,
        0x00000FF0,
        0x03C00FF0,
        0x3FC00FF0,
        0x3FE00FF0,
        0x3FE01FF0,
        0x1FF03FE0,
        0x1FF87FE0,
        0x0FFFFFC0,
        0x0FFFFFC0,
        0x07FFFF80,
        0x03FFFF00,
        0x01FFFE00,
        0x00FFF800,
        0x001FE000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
    ],
    '4': [
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00003F80,
        0x00007F80,
        0x0000FF80,
        0x0000FF80,
        0x0001FF80,
        0x0003FF80,
        0x0007FF80,
        0x0007FF80,
        0x000FFF80,
        0x001FFF80,
        0x001FFF80,
        0x003FFF80,
        0x007F7F80,
        0x00FF7F80,
        0x00FE7F80,
        0x01FC7F80,
        0x03F87F80,
        0x03F87F80,
        0x07F07F80,
        0x0FE07F80,
        0x1FE07F80,
        0x1FC07F80,
        0x3F807F80,
        0x7F807F80,
        0x7FFFFFFC,
        0x7FFFFFFC,
        0x7FFFFFFC,
        0x7FFFFFFC,
        0x7FFFFFFC,
        0x7FFFFFFC,
        0x7FFFFFFC,
        0x00007F80,
        0x00007F80,
        0x00007F80,
        0x00007F80,
        0x00007F80,
        0x00007F80,
        0x00007F80,
        0x00007F80,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
    ],
    '5': [
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x01FFFFE0,
        0x01FFFFE0,
        0x01FFFFE0,
        0x03FFFFE0,
        0x03FFFFE0,
        0x03FFFFE0,
        0x03FFFFE0,
        0x03FFFFE0,
        0x03F80000,
        0x07F80000,
        0x07F80000,
        0x07F80000,
        0x07F80000,
        0x07F00000,
        0x0FF3F800,
        0x0FFFFE00,
        0x0FFFFF00,
        0x0FFFFFC0,
        0x0FFFFFC0,
        0x0FFFFFE0,
        0x1FFFFFF0,
        0x1FF83FF0,
        0x1FE01FF0,
        0x03C00FF8,
        0x000007F8,
        0x000007F8,
        0x000007F8,
        0x07C007F8,
        0x3FC007F8,
        0x3FE007F8,
        0x1FE00FF0,
        0x1FF01FF0,
        0x1FF83FF0,
        0x0FFFFFE0,
        0x0FFFFFC0,
        0x07FFFFC0,
        0x03FFFF80,
        0x01FFFF00,
        0x007FFC00,
        0x001FE000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
    ],
    '6': [
        0x00000000,
        0x00000000,
        0x00000000,
        0x000FF000,
        0x003FFC00,
        0x00FFFE00,
        0x01FFFF00,
        0x03FFFF80,
        0x07FFFFC0,
        0x07FFFFC0,
        0x0FF87FC0,
        0x0FF03FE0,
        0x1FE01FE0,
        0x1FE01F00,
        0x1FC00000,
        0x1FC00000,
        0x1FC00000,
        0x3FC00000,
        0x3FC7F000,
        0x3FDFFC00,
        0x3FBFFE00,
        0x3FFFFF80,
        0x3FFFFF80,
        0x3FFFFFC0,
        0x3FFFFFE0,
        0x3FF87FE0,
        0x3FE01FE0,
        0x3FE01FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x1FC00FF0,
        0x1FC00FF0,
        0x1FC00FF0,
        0x1FE01FF0,
        0x0FF01FE0,
        0x0FF87FE0,
        0x07FFFFE0,
        0x07FFFFC0,
        0x03FFFF80,
        0x01FFFF00,
        0x00FFFE00,
        0x007FFC00,
        0x000FE000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
    ],
    '7': [
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x3FFFFFF0,
        0x3FFFFFF0,
        0x3FFFFFF0,
        0x3FFFFFF0,
        0x3FFFFFF0,
        0x3FFFFFF0,
        0x3FFFFFE0,
        0x3FFFFFE0,
        0x00001FC0,
        0x00003F80,
        0x00007F00,
        0x00007F00,
        0x0000FE00,
        0x0001FC00,
        0x0001FC00,
        0x0003F800,
        0x0003F800,
        0x0007F000,
        0x000FF000,
        0x000FE000,
        0x000FE000,
        0x001FE000,
        0x001FC000,
        0x003FC000,
        0x003F8000,
        0x003F8000,
        0x007F8000,
        0x007F8000,
        0x007F0000,
        0x00FF0000,
        0x00FF0000,
        0x00FF0000,
        0x00FF0000,
        0x01FE0000,
        0x01FE0000,
        0x01FE0000,
        0x01FE0000,
        0x01FE0000,
        0x01FE0000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
    ],
    '8': [
        0x00000000,
        0x00000000,
        0x00000000,
        0x001FE000,
        0x00FFFC00,
        0x01FFFE00,
        0x03FFFF00,
        0x07FFFF80,
        0x0FFFFFC0,
        0x0FF87FC0,
        0x1FF03FE0,
        0x1FE01FE0,
        0x1FE01FE0,
        0x1FE01FE0,
        0x1FE01FE0,
        0x1FE01FE0,
        0x1FE01FC0,
        0x0FF03FC0,
        0x07F87F80,
        0x07FFFF80,
        0x01FFFE00,
        0x00FFFC00,
        0x00FFFC00,
        0x03FFFF00,
        0x07FFFF80,
        0x0FF87FC0,
        0x1FF03FE0,
        0x1FE01FE0,
        0x3FE00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FE00FF0,
        0x3FE01FF0,
        0x1FF03FE0,
        0x1FF87FE0,
        0x0FFFFFC0,
        0x07FFFF80,
        0x07FFFF80,
        0x01FFFE00,
        0x00FFFC00,
        0x001FE000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
    ],
    '9': [
        0x00000000,
        0x00000000,
        0x00000000,
        0x001FC000,
        0x00FFF000,
        0x01FFFC00,
        0x03FFFE00,
        0x07FFFF00,
        0x0FFFFF80,
        0x1FFFFF80,
        0x1FF07FC0,
        0x1FE03FC0,
        0x3FE01FE0,
        0x3FC01FE0,
        0x3FC00FE0,
        0x3FC00FE0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FC00FF0,
        0x3FE01FF0,
        0x1FE01FF0,
        0x1FF87FF0,
        0x1FFFFFF0,
        0x0FFFFFF0,
        0x07FFFFF0,
        0x07FFFFF0,
        0x03FFF7F0,
        0x00FFEFF0,
        0x003F8FF0,
        0x00000FF0,
        0x00000FE0,
        0x00000FE0,
        0x00000FE0,
        0x03E01FE0,
        0x1FE01FC0,
        0x1FF03FC0,
        0x0FF87FC0,
        0x0FFFFF80,
        0x0FFFFF80,
        0x07FFFF00,
        0x03FFFE00,
        0x01FFFC00,
        0x00FFF000,
        0x003FC000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
    ],
    '-': [
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x007FFF00,
        0x007FFF00,
        0x007FFF00,
        0x007FFF00,
        0x007FFF00,
        0x007FFF00,
        0x007FFF00,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
    ],
}

class FrameBuffer:
    def __init__(self, width=160, height=128):
        self.width = width
        self.height = height
        self.buffer = bytearray(width * height * 2)

    def fill(self, color):
        c1, c2 = (color >> 8) & 0xFF, color & 0xFF
        self.buffer[:] = bytes([c1, c2]) * (self.width * self.height)

    def draw_pixel(self, x, y, color):
        if 0 <= x < self.width and 0 <= y < self.height:
            idx = (y * self.width + x) * 2
            self.buffer[idx] = (color >> 8) & 0xFF
            self.buffer[idx+1] = color & 0xFF

    def draw_text(self, x, y, text, color, scale=2, outline=True, outline_color=0x0000):
        char_width = 5
        height = 7 * scale
        
        offsets = [(0,0)]
        if outline:
            if isinstance(outline, bool):
                o = max(1, scale // 2)
            else:
                o = int(outline)
            offsets = [(-o, -o), (0, -o), (o, -o), (-o, 0), (o, 0), (-o, o), (0, o), (o, o), (0, 0)]
            
        is_gradient = isinstance(color, tuple)
            
        for ox, oy in offsets:
            is_outline = (ox != 0 or oy != 0)
            if not is_outline and not is_gradient:
                fg_high = (color >> 8) & 0xFF
                fg_low = color & 0xFF
            
            for row in range(height):
                if is_outline:
                    fg_high = (outline_color >> 8) & 0xFF
                    fg_low = outline_color & 0xFF
                elif is_gradient:
                    ratio = row / max(1, height - 1)
                    c = interpolate_color(color[0], color[1], ratio)
                    fg_high = (c >> 8) & 0xFF
                    fg_low = c & 0xFF

                font_row = row // scale
                curr_x = x + ox
                
                for char in text:
                    pattern = FONT_5x7.get(char, FONT_5x7[' '])[font_row]
                    for col in range(char_width):
                        bit = (pattern >> (4 - col)) & 1
                        if bit:
                            for dx in range(scale):
                                px = curr_x + dx
                                py = y + row + oy
                                if 0 <= px < self.width and 0 <= py < self.height:
                                    idx = (py * self.width + px) * 2
                                    self.buffer[idx] = fg_high
                                    self.buffer[idx+1] = fg_low
                        curr_x += scale
                    curr_x += scale * 2 # increased spacing

    def draw_text_centered(self, y, text, color, scale=2, outline=True):
        char_width = 5
        width_pixels = len(text) * (char_width + 2) * scale
        base_start_x = max(0, (self.width - width_pixels) // 2)
        self.draw_text(base_start_x, y, text, color, scale, outline)

    def draw_highres_text(self, x, y, text, color, outline=True, outline_color=0x0000):
        width = 32
        height = 48
        
        offsets = [(0,0)]
        if outline:
            o = int(outline) if not isinstance(outline, bool) else 2
            offsets = [(-o, -o), (0, -o), (o, -o), (-o, 0), (o, 0), (-o, o), (0, o), (o, o), (0, 0)]
            
        is_gradient = isinstance(color, tuple)
            
        for ox, oy in offsets:
            is_outline = (ox != 0 or oy != 0)
            if not is_outline and not is_gradient:
                fg_high = (color >> 8) & 0xFF
                fg_low = color & 0xFF
            
            for row in range(height):
                if is_outline:
                    fg_high = (outline_color >> 8) & 0xFF
                    fg_low = outline_color & 0xFF
                elif is_gradient:
                    ratio = row / max(1, height - 1)
                    c = interpolate_color(color[0], color[1], ratio)
                    fg_high = (c >> 8) & 0xFF
                    fg_low = c & 0xFF

                curr_x = x + ox
                
                for char in text:
                    pattern_row = FONT_32x48.get(char, [0]*48)[row]
                    for col in range(width):
                        bit = (pattern_row >> (31 - col)) & 1
                        if bit:
                            px = curr_x
                            py = y + row + oy
                            if 0 <= px < self.width and 0 <= py < self.height:
                                idx = (py * self.width + px) * 2
                                self.buffer[idx] = fg_high
                                self.buffer[idx+1] = fg_low
                        curr_x += 1
                    curr_x += 2 # spacing

class WeatherAnimation:
    def _random_x(self):
        # Spawn particles only on the far left (x=2 to x=38) to strictly avoid the temperature text and deg/C
        return random.randint(2, 38)

    def __init__(self):
        self.tick = 0
        self.start_time = time.time()
        
        # Precompute large snowflake pixel offsets
        self.large_flake_pixels = []
        radius = 8
        for i in range(8):
            angle = i * 45 * math.pi / 180
            for r in range(radius):
                rx = int(math.cos(angle) * r)
                ry = int(math.sin(angle) * r)
                self.large_flake_pixels.append((rx, ry))
        for i in range(4):
            angle = i * 90 * math.pi / 180
            fork_r = 4
            fx = int(math.cos(angle) * fork_r)
            fy = int(math.sin(angle) * fork_r)
            left_angle = angle + 45 * math.pi / 180
            right_angle = angle - 45 * math.pi / 180
            for d in range(1, 4):
                self.large_flake_pixels.append((int(fx + math.cos(left_angle)*d), int(fy + math.sin(left_angle)*d)))
                self.large_flake_pixels.append((int(fx + math.cos(right_angle)*d), int(fy + math.sin(right_angle)*d)))
        self.large_flake_pixels = list(set(self.large_flake_pixels))
        
        # Spread particles evenly vertically so they never clump/overlap
        self.drops = [(self._random_x(), i * (128 // 8)) for i in range(8)]
        self.flakes = [(self._random_x(), i * (128 // 4), random.randint(1, 2)) for i in range(4)]
        self.stars = [(random.randint(0, 159), random.randint(0, 127)) for _ in range(30)]
        self.shooting_star = None
        self.sun_radius = 20
        self.cloud_offset = 0

    def draw(self, fb, desc, is_night=False):
        desc = desc.upper()
        self.tick += 1
        
        if desc == 'FETCHING...':
            fb.fill(0x0000)
            return
            
        if 'SUN' in desc or 'CLEAR' in desc:
            fb.fill(0x0000) # Deep Space / Black for clock clarity
            if is_night:
                # Draw stars
                for i, (sx, sy) in enumerate(self.stars):
                    if (self.tick + i * 7) % 15 < 3:
                        fb.draw_pixel(sx, sy, 0xFFFF)
                    else:
                        fb.draw_pixel(sx, sy, 0x8410)
                
                # Draw a crescent moon
                center_x, center_y = 80, 64
                radius = 45
                for y in range(center_y - radius, center_y + radius):
                    for x in range(center_x - radius, center_x + radius):
                        dist_sq = (x - center_x)**2 + (y - center_y)**2
                        if dist_sq <= radius**2:
                            # Inner subtractive circle
                            dist_sq_sub = (x - (center_x + 15))**2 + (y - (center_y - 15))**2
                            if dist_sq_sub > (radius - 7)**2:
                                fb.draw_pixel(x, y, 0xFFE0) # Yellow moon
                                
                # Occasional flaming comet
                if self.shooting_star:
                    sx = int(self.shooting_star['x'])
                    sy = int(self.shooting_star['y'])
                    
                    # Core (White/Yellow, 2x2)
                    fb.draw_pixel(sx, sy, 0xFFFF)
                    fb.draw_pixel(sx+1, sy, 0xFFE0)
                    fb.draw_pixel(sx, sy+1, 0xFFE0)
                    fb.draw_pixel(sx+1, sy+1, 0xFFFF)
                    
                    # Flaming Tail (Orange to Red)
                    for d in range(1, 7):
                        tx = sx - int(self.shooting_star['dx'] * d * 0.5)
                        ty = sy - int(self.shooting_star['dy'] * d * 0.5)
                        color = 0xFD20 if d < 4 else 0xF800 # Orange then Red
                        fb.draw_pixel(tx, ty, color)
                        # Thicken the tail near the core
                        if d < 4:
                            fb.draw_pixel(tx+1, ty, color)
                            fb.draw_pixel(tx, ty+1, color)
                    
                    self.shooting_star['x'] += self.shooting_star['dx']
                    self.shooting_star['y'] += self.shooting_star['dy']
                    self.shooting_star['life'] -= 1
                    
                    if self.shooting_star['life'] <= 0 or sx < 0 or sx > 159 or sy > 127:
                        self.shooting_star = None
                else:
                    if random.random() < 0.01: # roughly every 10 seconds
                        self.shooting_star = {
                            'x': random.randint(120, 159),
                            'y': random.randint(0, 20),
                            'dx': random.uniform(-2, -4), # Slower and majestic
                            'dy': random.uniform(1, 3),
                            'life': 80
                        }
            else:
                # Draw a big sun in the center
                center_x, center_y = 80, 64
                # Pulse radius (larger, slower pulse)
                radius = 45 + int(math.sin(self.tick * 0.05) * 8)
                r_sq = radius**2
                r1_sq = (radius * 0.4)**2
                r2_sq = (radius * 0.7)**2
                
                for y in range(center_y - radius, center_y + radius):
                    for x in range(center_x - radius, center_x + radius):
                        dist_sq = (x - center_x)**2 + (y - center_y)**2
                        if dist_sq <= r_sq:
                            if dist_sq < r1_sq:
                                color = 0xFFFF # White core
                            elif dist_sq < r2_sq:
                                color = 0xFFE0 # Yellow mid
                            else:
                                color = 0xFD20 # Orange edge
                            fb.draw_pixel(x, y, color)
                # Draw rays (slower rotation)
                for i in range(12):
                    angle = (i * 30 + self.tick * 0.5) * math.pi / 180
                    for r in range(radius + 5, radius + 35):
                        rx = int(center_x + math.cos(angle) * r)
                        ry = int(center_y + math.sin(angle) * r)
                        fb.draw_pixel(rx, ry, 0xFD20)
                        fb.draw_pixel(rx+1, ry, 0xFD20)
                        fb.draw_pixel(rx, ry+1, 0xFD20)
                        fb.draw_pixel(rx+1, ry+1, 0xFD20)

        elif 'RAIN' in desc or 'DRIZZLE' in desc or 'SHOWER' in desc:
            fb.fill(0x0000)
            # Large teardrop
            drop_pixels = [(0,0), (0,1), 
                           (-1,2), (0,2), (1,2), 
                           (-2,3), (-1,3), (0,3), (1,3), (2,3), 
                           (-2,4), (-1,4), (0,4), (1,4), (2,4), 
                           (-1,5), (0,5), (1,5)]
            for i in range(len(self.drops)):
                x, y = self.drops[i]
                for dx, dy in drop_pixels:
                    fb.draw_pixel(x + dx, y + dy, 0x03FF) # Deep watery blue
                new_y = y + 2 # slower rain
                if new_y > 128:
                    new_y = -5
                    x = self._random_x()
                self.drops[i] = (x, new_y)

        elif 'SNOW' in desc or 'ICE' in desc or 'BLIZZARD' in desc:
            fb.fill(0x0000)
            for i in range(len(self.flakes)):
                x, y, speed = self.flakes[i]
                for dx, dy in self.large_flake_pixels:
                    fb.draw_pixel(x + dx, y + dy, 0xFFFF)
                    
                # Slower parallax falling
                if self.tick % (3 - speed) == 0:
                    new_y = y + 1
                else:
                    new_y = y
                    
                new_x = x + int(math.sin(self.tick * 0.1 + i))
                if new_y > 128 + 10:
                    new_y = -10
                    new_x = self._random_x()
                self.flakes[i] = (new_x, new_y, speed)

        else: # Cloudy
            fb.fill(0x0000)
            self.cloud_offset = int(math.sin(self.tick * 0.1) * 10)
            # Draw a massive cloud
            circles = [(80, 80, 45), (40, 90, 35), (120, 90, 35), (55, 65, 40), (105, 65, 35)]
            for cx, cy, cr in circles:
                cy += self.cloud_offset
                for y in range(cy - cr, cy + cr):
                    for x in range(cx - cr, cx + cr):
                        if (x - cx)**2 + (y - cy)**2 <= cr**2:
                            # Draw alternating gray pixels for a dithering effect (transparency)
                            if (x + y) % 2 == 0:
                                fb.draw_pixel(x, y, 0x7BEF) # Light gray

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

    def blit(self, fb):
        if 'RPi.GPIO' not in sys.modules:
            return
            
        self.send_command(0x2A)
        self.send_data([0x00, 0x00, 0x00, 0x9F]) # 159
        self.send_command(0x2B)
        self.send_data([0x00, 0x00, 0x00, 0x7F]) # 127
        self.send_command(0x2C) # RAMWR
        
        GPIO.output(self.dc_pin, GPIO.HIGH)
        
        # Blast the entire buffer
        for i in range(0, len(fb.buffer), 4096):
            self.spi.xfer2(fb.buffer[i:i+4096])

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
        last_utc_second = -1
        
        # Set to True to cycle weather animations every 5 seconds
        DEMO_MODE = False
        
        while self.running:
            if DEMO_MODE:
                # TEST OVERRIDE: Cycle every 5s
                test_modes = ["SUNNY", "RAIN", "SNOW", "CLOUDY"]
                self.weather_data['desc'] = test_modes[int(time.time() / 5) % len(test_modes)]
            
            utc_now = datetime.now(timezone.utc)
            
            # 1. Update LED Matrices (Only when the physical second changes)
            if utc_now.second != last_utc_second:
                manila_tz = timezone(timedelta(hours=8))
                
                now = datetime.now()
                manila_time = utc_now.astimezone(manila_tz)
                
                digits_top = self.format_time_digits(now)
                digits_bot = self.format_time_digits(manila_time)
                
                # Write to the MAX7219 chain (8 registers/digits)
                for i in range(8):
                    register = 8 - i
                    self.chain.write_cmd_chain(register, [digits_top[i], digits_bot[i]])
                
                last_utc_second = utc_now.second
                
            # 2. Update TFT Display (Run at 10 FPS for fluid animations)
            self.fb.fill(0x0000)
            
            hour = datetime.now().hour
            is_night = (hour < 6 or hour >= 18)
            
            # Draw Weather Background
            desc = self.weather_data.get('desc', 'SUNNY')
            self.animator.draw(self.fb, desc, is_night=is_night)
                
            # Draw Text on Top (Transparent background with Black Outline)
            
            # Format and draw Temperature
            temp_val = self.weather_data.get('temp', '0').replace('C', '').strip()
            
            desc = self.weather_data.get('desc', 'SUNNY').upper()
            outline_color = 0x0000 # default black
            if "RAIN" in desc or "DRIZZLE" in desc or "THUNDER" in desc:
                temp_color = (0x07FF, 0x001F) # Cyan to Blue
                unit_color = (0x07FF, 0x001F)
                outline_color = 0xFFFF # Light border
            elif "SNOW" in desc:
                temp_color = (0xFFFF, 0x841F) # White to Light Blue
                unit_color = (0xFFFF, 0x841F)
                outline_color = 0xFFFF # Light border
            elif "CLOUD" in desc:
                temp_color = (0xCE79, 0xFFFF) # Light Gray to White
                unit_color = (0xCE79, 0xFFFF)
            else: # SUNNY / CLEAR
                if is_night:
                    temp_color = (0x07FF, 0xF81F) # Cool Cyan to Magenta for night
                    unit_color = (0x07FF, 0xF81F)
                else:
                    temp_color = (0xFFE0, 0xFC00) # Yellow to Orange
                    unit_color = (0xFFE0, 0xFC00)
            
            # Using our new high-res smooth font
            char_width_hr = 32
            spacing_hr = 3
            num_w = len(temp_val) * (char_width_hr + spacing_hr)
            
            # For the unit, we still use the 5x7 font at scale=2
            unit_scale = 2
            char_width_unit = 5
            unit_w = 2 * (char_width_unit + 2) * unit_scale
            
            # Center the number, but nudge it 4 pixels right to balance the visual weight of the degree symbol
            base_x = max(0, (160 - num_w) // 2) + 4
            # Safely clamp it so the degree symbol doesn't clip off the right edge of the screen!
            num_start_x = min(160 - num_w - unit_w, base_x)
            
            # Vertically center the temperature (the font is 48px tall, screen is 128px)
            temp_y = 40
            self.fb.draw_highres_text(num_start_x, temp_y, temp_val, temp_color, outline=1, outline_color=outline_color)
            
            # The user requested the °C to be at the bottom of the main text, rather than top.
            # High-res font is 48px tall. Unit font is scale=2 (14px tall).
            # Bottom align: temp_y + 48 - 14 = temp_y + 34
            unit_y = temp_y + 34
            self.fb.draw_text(num_start_x + num_w, unit_y, "°C", unit_color, scale=unit_scale, outline=1, outline_color=outline_color)
            
            # Humidity below
            self.fb.draw_text_centered(112, self.weather_data['hum'], (0xFFE0, 0x07E0), scale=1) # Yellow to Green
            
            # Blast to SPI
            self.tft.blit(self.fb)
                
            # Sleep until the top of the next 0.1 second (10 FPS tick)
            next_tick = 0.1 - (time.time() % 0.1)
            time.sleep(next_tick)

def main():
    # Setup MAX7219 on pure software SPI (Bitbang) to prevent hardware conflict
    # DIN = GPIO 17 (Pin 11), CLK = GPIO 18 (Pin 12), CS = GPIO 23 (Pin 16)
    chain = MAX7219Chain(din_pin=17, clk_pin=18, cs_pin=23, num_displays=2)
    
    # Setup TFT on native hardware SPI (spidev)
    # The kernel natively toggles CS via GPIO 22 based on our /boot/config.txt overlay.
    tft = ST7735(spi_bus=0, spi_device=0, dc_pin=24, rst_pin=25)
    
    # Initialize Clock with both displays
    clock = Clock(chain, tft)
    clock.fb = FrameBuffer()
    clock.animator = WeatherAnimation()

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

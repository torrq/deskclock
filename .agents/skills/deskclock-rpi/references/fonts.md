# Font System Reference

## Overview

The deskclock uses three custom bitmap font arrays, all pre-rasterized into C header files.
No TTF rendering happens at runtime — this is by design for performance and to avoid
Pillow/PIL dependencies on the Pi.

## Font Inventory

### 1. FONT_5X7 (`fonts.h`)
- **Size**: 5 pixels wide × 7 pixels tall
- **Source**: Extracted from `FONT_5x7` dict in `legacy/clock.py` via AST parsing
- **Used for**: Temperature unit text ("°C"), small labels
- **Encoding**: `uint8_t[N][7]` — each row is a 5-bit pattern (MSB = leftmost pixel)
- **Lookup**: Linear search via `CHARS_5X7[]` character array

### 2. FONT_32X48 (`fonts.h`)
- **Size**: 32 pixels wide × 48 pixels tall
- **Source**: Extracted from `FONT_32x48` dict in `legacy/clock.py` via AST parsing
- **Used for**: Large temperature digits with gradient fill and outline
- **Encoding**: `uint32_t[N][48]` — each row is a 32-bit bitmask
- **Lookup**: Linear search via `CHARS_32X48[]` character array
- **Characters**: Digits 0-9, minus sign, space

### 3. FONT_8X12 (`font_8x12.h`)
- **Size**: 8 pixels wide × 12 pixels tall
- **Source**: Generated from Consolas TTF (size 11) using Pillow on Windows
- **Used for**: Humidity text ("HUM XX%") with yellow→green gradient
- **Encoding**: `uint8_t[96][12]` — ASCII 32-127, each row is an 8-bit pattern
- **Lookup**: Direct index: `(int)char - 32`

## Regenerating Fonts

### 5x7 and 32x48 (from legacy/clock.py)
Use the export script to parse `legacy/clock.py`'s font dictionaries via Python AST:
```bash
python scratch/export_fonts.py
```
This outputs `src/fonts.h`.

### 8x12 (from TTF)
Use the generation script (requires Pillow):
```bash
python scratch/generate_8x12.py
```
This outputs `src/font_8x12.h`.

## Rendering Functions

| Function | Font | File | Features |
|---|---|---|---|
| `tft_draw_text()` | 5x7 | `tft.c` | Solid color, integer scaling |
| `tft_draw_text_gradient()` | 8x12 | `tft.c` | Per-row color interpolation (gradient) |
| `tft_draw_highres_text()` | 32x48 | `tft.c` | Gradient fill + 1px outline |

## Adding Characters

1. For 5x7/32x48: Add the glyph to the Python dict in `legacy/clock.py`, re-run `export_fonts.py`
2. For 8x12: The full ASCII 32-127 range is already covered
3. **Always** ensure displayed text only contains mapped characters — unmapped chars cause out-of-bounds reads

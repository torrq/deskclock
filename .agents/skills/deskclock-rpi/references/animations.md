# Animation System Reference

## Overview

All weather animations are implemented in [anim.c](file:///c:/Users/nathan/Projects/git/deskclock/src/anim.c).
The animation system uses a global `tick` counter incremented each frame (10 FPS = 100ms per tick).

## Particle System

### Data Structures

```c
typedef struct { int x, y, speed; } Particle;   // Rain drops and snow flakes
typedef struct { int x, y; } Star;               // Background stars
typedef struct { float x, y, dx, dy; int life; bool active; } Comet;
typedef struct { int x, y; } Point;              // Pixel offset for shapes
```

### Rain (8 drops)
- Shape: 18-pixel teardrop defined in `drop_pixels[][]` static array
- Movement: Falls at 2px/tick, respawns at y=-5 when off-screen
- Color: `0x03FF` (bright cyan)

### Snow (4 flakes)
- Shape: 8-arm snowflake with forked tips, generated at init via trig
- Stored in `large_flake_pixels[]` (up to 100 points)
- Movement: Speed 1-2, horizontal sine-wave drift
- Color: `0xFFFF` (white)

### Stars (30 stars)
- Twinkling effect: brightness alternates based on `(tick + i*7) % 15`
- Bright: `0xFFFF`, dim: `0x8410`

### Comet
- 1% chance per tick to spawn
- 2x2 head with 7-pixel tail (orange→red gradient)
- Travels diagonally with velocity `dx`, `dy`
- Despawns after 80 ticks or leaving screen bounds

## Animation Dispatch

The `anim_draw()` function selects animation based on `strstr()` keyword matching
on the uppercase weather description:

```c
if      FETCHING         → black screen
else if SUN/CLEAR        → night ? stars+comet : pulsing sun
else if RAIN/DRIZZLE     → falling teardrops
else if SNOW/ICE         → drifting snowflakes  
else                     → bouncing cloud (default)
```

## Sun Animation
- Center: (80, 64)
- Radius: 45 ± 8px (pulsing via sine wave)
- 3-zone color: white core → yellow middle → orange edge
- 12 rotating rays (2px wide, rotate at 0.5°/tick)

## Cloud Animation
- 5 overlapping filled circles with stipple pattern (`(x+y)%2`)
- Vertical bounce via sine wave offset
- Color: `0x7BEF` (medium gray)

## Adding a New Animation

1. Define any new particle arrays as `static` in `anim.c`
2. Initialize them in `anim_init()`
3. Add a new `else if (strstr(desc, "KEYWORD"))` branch in `anim_draw()`
4. Always call `tft_fill(0x0000)` first to clear the background
5. Add matching temperature color scheme in `main.c`
6. Test with demo mode: set `DEMO_MODE = true` and add keyword to `test_modes[]`

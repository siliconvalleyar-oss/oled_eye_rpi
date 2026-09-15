#!/usr/bin/env python3
import numpy as np
from PIL import Image

# Open framebuffer
with open('/dev/fb0', 'wb') as fb:
    # Create a test pattern (128x64, 1-bit)
    img = Image.new('1', (128, 64), 0)
    pixels = img.load()
    
    # Draw some test patterns
    # Horizontal lines
    for y in range(0, 64, 8):
        for x in range(128):
            pixels[x, y] = 1
    
    # Vertical lines
    for x in range(0, 128, 8):
        for y in range(64):
            pixels[x, y] = 1
    
    # Draw a border
    for x in range(128):
        pixels[x, 0] = 1
        pixels[x, 63] = 1
    for y in range(64):
        pixels[0, y] = 1
        pixels[127, y] = 1
    
    # Convert to bytes and write to framebuffer
    # The framebuffer expects data in a specific format
    # For 1-bit 128x64, each row is 128/8 = 16 bytes
    fb_data = np.array(img, dtype=np.uint8)
    fb_bytes = np.packbits(fb_data, axis=1)
    fb.write(fb_bytes.tobytes())

print("Test pattern written to OLED")

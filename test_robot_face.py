#!/usr/bin/env python3
import numpy as np
from PIL import Image, ImageDraw

# Open framebuffer
with open('/dev/fb0', 'wb') as fb:
    # Create a test image (128x64, 1-bit monochrome)
    img = Image.new('1', (128, 64), 0)
    draw = ImageDraw.Draw(img)
    
    # Draw a simple robot face
    # Eyes (two circles)
    draw.ellipse([20, 15, 50, 45], outline=1, fill=0)
    draw.ellipse([78, 15, 108, 45], outline=1, fill=0)
    
    # Pupils (smaller circles inside)
    draw.ellipse([30, 25, 40, 35], outline=1, fill=1)
    draw.ellipse([88, 25, 98, 35], outline=1, fill=1)
    
    # Mouth (a simple line/rectangle)
    draw.rectangle([40, 50, 88, 55], outline=1, fill=1)
    
    # Convert to framebuffer format and write
    fb_data = np.array(img, dtype=np.uint8)
    fb_bytes = np.packbits(fb_data, axis=1)
    fb.write(fb_bytes.tobytes())

print("Robot face displayed on OLED")

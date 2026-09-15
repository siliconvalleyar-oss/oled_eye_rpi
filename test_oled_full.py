#!/usr/bin/env python3
"""
Test script for SSD1306 OLED on Raspberry Pi
Uses the framebuffer device created by the ssd1306 overlay
"""
import numpy as np
from PIL import Image, ImageDraw
import time

def write_framebuffer(img):
    """Write a PIL Image to the framebuffer device"""
    with open('/dev/fb0', 'wb') as fb:
        fb_data = np.array(img, dtype=np.uint8)
        fb_bytes = np.packbits(fb_data, axis=1)
        fb.write(fb_bytes.tobytes())

def main():
    print("Starting OLED test...")
    
    # Create image
    img = Image.new('1', (128, 64), 0)
    draw = ImageDraw.Draw(img)
    
    # Test 1: Display text
    print("Test 1: Displaying text...")
    draw.text((10, 10), "OLED Working!", fill=1)
    draw.text((10, 25), "128x64", fill=1)
    draw.text((10, 40), "Address: 0x3c", fill=1)
    write_framebuffer(img)
    time.sleep(2)
    
    # Test 2: Draw shapes
    print("Test 2: Drawing shapes...")
    draw.rectangle([0, 0, 127, 63], outline=1, fill=0)
    draw.ellipse([20, 10, 50, 40], outline=1, fill=0)
    draw.rectangle([70, 10, 110, 40], outline=1, fill=1)
    draw.text((40, 50), "Test OK", fill=1)
    write_framebuffer(img)
    time.sleep(2)
    
    # Test 3: Animation - bouncing ball
    print("Test 3: Animation test...")
    x, y = 10, 10
    dx, dy = 2, 1
    for _ in range(50):
        draw.rectangle([0, 0, 127, 63], fill=0)  # Clear
        draw.ellipse([x-3, y-3, x+3, y+3], fill=1)  # Draw ball
        write_framebuffer(img)
        
        # Update position
        x += dx
        y += dy
        if x <= 3 or x >= 124:
            dx = -dx
        if y <= 3 or y >= 60:
            dy = -dy
        time.sleep(0.05)
    
    # Final message
    print("Test 4: Final message...")
    draw.rectangle([0, 0, 127, 63], fill=0)
    draw.text((30, 25), "SUCCESS!", fill=1)
    write_framebuffer(img)
    
    print("All tests completed!")

if __name__ == "__main__":
    main()

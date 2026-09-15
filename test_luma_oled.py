#!/usr/bin/env python3
from luma.core.interface.serial import i2c
from luma.core.render import canvas
from luma.oled.device import ssd1306

# Initialize I2C and OLED
serial = i2c(port=1, address=0x3c)
device = ssd1306(serial)

# Display test pattern
with canvas(device) as draw:
    # Draw border
    draw.rectangle(device.bounding_box, outline="white", fill="black")
    
    # Draw some shapes
    draw.ellipse([10, 10, 50, 50], outline="white", fill="black")
    draw.rectangle([60, 10, 100, 50], outline="white", fill="black")
    
    # Draw text
    draw.text((10, 55), "OLED Test", fill="white")

print("Test pattern displayed on OLED")

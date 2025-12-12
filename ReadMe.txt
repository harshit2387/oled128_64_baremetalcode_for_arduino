In my bare‑metal programming experiments, I used a linked list to manage pixel data on an I²C OLED display, which gave me a clear understanding of how the hardware works from scratch. 
The display has a resolution of 128×64, meaning it contains 8192 individual pixels stored across 1024 bytes of memory. 
Each pixel can be controlled by calculating its page and bit position in the frame buffer, and once updated, the buffer is sent over I²C to refresh the screen. 
By setting and clearing pixels in sequence, I was able to animate a single point across the display, and the linked list structure made it easier to extend this idea into dynamic shapes or sprites. 
This is just the beginning of building a custom graphics stack, starting with one pixel and gradually moving toward more complex animations and visual effects

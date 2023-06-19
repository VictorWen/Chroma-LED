import board
import neopixel
import numpy as np
import sys

n = 150
n_channels = 4
dtype = np.float32
dtype_size = 4
max_color = 255

def write_to_pixels(pixels, frame):
    for i in range(len(pixels)):
        alpha = frame[i][3]
        red = frame[i][0]
        green = frame[i][1]
        blue = frame[i][2]
        pixels[i] = ((int) (red * alpha * max_color),
                     (int) (green * alpha * max_color),
                     (int) (blue * alpha * max_color))
    pixels.show()

def main():
    pixels = neopixel.NeoPixel(board.D10, n, brightness=0.5, auto_write=False)
    
    frame_size = n * n_channels * dtype_size

    while True:
        frame = sys.stdin.buffer.read(frame_size)
        frame = np.frombuffer(frame, dtype=dtype).reshape(n, n_channels)
        write_to_pixels(pixels, frame)
        
    
if __name__ == "__main__":
    main()
        
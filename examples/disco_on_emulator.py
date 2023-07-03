import os
abspath = os.path.realpath(__file__)
dname = os.path.dirname(abspath)
os.chdir(dname)

os.chdir("./NeoPixelEmulator/")

import sys
sys.path.append('./')

from emulator_backend import Adafruit_NeoPixel # Use https://github.com/having11/NeoPixel-Emulator
import numpy as np
import sys

n = 150
n_channels = 4
dtype = np.float32
dtype_size = 4
max_color = 255


def write_to_pixels(pixels, frame):
    for i in range(n):
        alpha = 1
        red = frame[i][0]
        green = frame[i][1]
        blue = frame[i][2]
        color = ((int)(red * alpha * max_color),
                 (int)(green * alpha * max_color),
                 (int)(blue * alpha * max_color))
        pixels.setPixelColor(i, color)
    pixels.show()


def main():
    pixels = Adafruit_NeoPixel(n, 1, "")
    pixels.begin()
    pixels.setBrightness(100)

    frame_size = n * n_channels * dtype_size

    try:
        while True:
            frame = sys.stdin.buffer.read(frame_size)
            # print(frame)
            frame = np.frombuffer(frame, dtype=dtype).reshape(n, n_channels)
            # print(frame)
            write_to_pixels(pixels, frame)
    except Exception as e:
        print(e)
        print(frame, len(frame))


if __name__ == "__main__":
    main()

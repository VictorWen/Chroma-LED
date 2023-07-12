import socket
import numpy as np

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


n=150
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


local_IP = "127.0.0.1"
local_port = 8080
buffer_size = 4096

res_msg = "Hello there!"
res_bytes = str.encode(res_msg)


def main():
    pixels = Adafruit_NeoPixel(n, 1, "")
    pixels.begin()
    pixels.setBrightness(100)

    server_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
    server_socket.bind((local_IP, local_port))

    print("UDP server running")

    while (True):
        req_bytes, client_addr = server_socket.recvfrom(buffer_size)
        start = int.from_bytes(req_bytes[0:4], "little")
        end = int.from_bytes(req_bytes[4:8], "little")
        n_frames = int.from_bytes(req_bytes[8:12], "little")
        size = end - start

        frame = np.frombuffer(req_bytes[12:], dtype=dtype).reshape(size, n_channels)
        print(f"received: {frame}, from: {client_addr}")

        server_socket.sendto(res_bytes, client_addr)
        
        write_to_pixels(pixels, frame)


if __name__ == "__main__":
    main()

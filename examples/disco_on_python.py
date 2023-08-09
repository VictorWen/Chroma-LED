import socket
import json
import sys
import os
import numpy as np

abspath = os.path.realpath(__file__)
dname = os.path.dirname(abspath)
os.chdir(dname)
os.chdir("./NeoPixelEmulator/")
sys.path.append('./')

from emulator_backend import Adafruit_NeoPixel


DISCOVER_PORT = 12346
BUFFER_SIZE = 4096

SCAN_MSG = "DISCO DISCOVER\n"
CONNECT_MSG = "DISCO CONNECT\n"
READY_MSG = "DISCO READY\n"
HARDWARE_DATA = {
    "controllerID": "TEST",
    "device": "windows",
    "discoVersion": 1,
}

PIXEL_LEN = 150
N_CHANNELS = 4
DTYPE = np.float32
DTYPE_SIZE = 4
COLOR_MAX = 255

DATA_PORT = 12345

RES_MSG = "Hello there!"
RES_BYTES = str.encode(RES_MSG)


def wait_for_discover(server_socket):
    req_bytes, client_addr = server_socket.recvfrom(BUFFER_SIZE)
    print(req_bytes)
    data = req_bytes.decode()
    print(f"received: {data}, from: {client_addr}")
    if data == SCAN_MSG:
        print("Got SCAN_DATA!")
        server_socket.sendto(
            f"DISCO FOUND\n{json.dumps(HARDWARE_DATA)}".encode(),
            client_addr
        )
        req_bytes, client_addr = server_socket.recvfrom(BUFFER_SIZE)
        print(req_bytes)
        data = req_bytes.decode()
        if (data.startswith(CONNECT_MSG)):
            json_data = json.loads(data[len(CONNECT_MSG):])
            print(f"received: {json.dumps(json_data, indent=2)}, " +
                  f"from: {client_addr}")
    return client_addr


def write_to_pixels(pixels, frame):
    for i in range(PIXEL_LEN):
        alpha = 1
        red = frame[i][0]
        green = frame[i][1]
        blue = frame[i][2]
        color = ((int)(red * alpha * COLOR_MAX),
                 (int)(green * alpha * COLOR_MAX),
                 (int)(blue * alpha * COLOR_MAX))
        pixels.setPixelColor(i, color)
    pixels.show()


def main():
    pixels = Adafruit_NeoPixel(PIXEL_LEN, 1, "")
    pixels.begin()
    pixels.setBrightness(100)

    discover_socket = socket.socket(
        family=socket.AF_INET,
        type=socket.SOCK_DGRAM
    )
    discover_socket.bind(('0.0.0.0', DISCOVER_PORT))

    master_addr = wait_for_discover(discover_socket)

    server_socket = socket.socket(
        family=socket.AF_INET,
        type=socket.SOCK_DGRAM
    )
    server_socket.bind(('0.0.0.0', DATA_PORT))

    discover_socket.sendto(READY_MSG.encode(), master_addr)
    discover_socket.close()

    print("Finished discovery")

    print("UDP server running")

    while (True):
        req_bytes, client_addr = server_socket.recvfrom(BUFFER_SIZE)
        start = int.from_bytes(req_bytes[0:4], "little")
        end = int.from_bytes(req_bytes[4:8], "little")
        # n_frames = int.from_bytes(req_bytes[8:12], "little")
        size = end - start

        frame = np.frombuffer(req_bytes[12:], dtype=DTYPE) \
            .reshape(size, N_CHANNELS)
        print(f"received: {frame}, from: {client_addr}")

        server_socket.sendto(RES_BYTES, client_addr)

        write_to_pixels(pixels, frame)


if __name__ == "__main__":
    main()
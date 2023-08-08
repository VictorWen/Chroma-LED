import socket
import json

PORT = 12346
BUFFER_SIZE = 4096

SCAN_DATA = "DISCO DISCOVER\n"
CONNECT_DATA = "DISCO CONNECT\n"
HARDWARE_DATA = {
    "controllerID": "TEST",
    "device": "windows",
    "discoVersion": 1,
}


def main():
    server_socket = socket.socket(
        family=socket.AF_INET, type=socket.SOCK_DGRAM
    )
    server_socket.bind(('0.0.0.0', PORT))
    
    req_bytes, client_addr = server_socket.recvfrom(BUFFER_SIZE)
    print(req_bytes)
    data = req_bytes.decode()
    print(f"received: {data}, from: {client_addr}")
    if data == SCAN_DATA:
        print("Got SCAN_DATA!")
        server_socket.sendto(
            f"DISCO FOUND\n{json.dumps(HARDWARE_DATA)}".encode(),
            client_addr
        )
        req_bytes, client_addr = server_socket.recvfrom(BUFFER_SIZE)
        print(req_bytes)
        data = req_bytes.decode()
        if (data.startswith(CONNECT_DATA)):
            json_data = json.loads(data[len(CONNECT_DATA):])
            print(f"received: {json.dumps(json_data, indent=2)}, " +
                  f"from: {client_addr}")


if __name__ == "__main__":
    main()

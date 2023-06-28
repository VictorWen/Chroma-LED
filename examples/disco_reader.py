import numpy as np
import sys

n = 150
n_channels = 4
dtype = np.float32
dtype_size = 4
max_color = 255

def main():    
    frame_size = n * n_channels * dtype_size

    while True:
        frame = sys.stdin.buffer.read(frame_size)
        print(frame)
        frame = np.frombuffer(frame, dtype=dtype).reshape(n, n_channels)
        print(frame)
        break
       
    
if __name__ == "__main__":
    main()
        

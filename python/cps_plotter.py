import socket
import struct
import threading
import time
import numpy as np
import matplotlib.pyplot as plt

l = threading.Lock()

NUM_ENTRY = 300
x_val = np.zeros( NUM_ENTRY * 4 )
y_val = np.zeros( NUM_ENTRY * 4 )

e = threading.Event()

def reading_func():
    s = socket.socket()
    s.connect( ( '1.1.1.2', 100 ) )

    while True:
        timestamp = struct.unpack( "<I", s.recv( 4 ) )[0]
        print( timestamp )
        with l:
            x_val[ :-4 ] = x_val[ 4: ]
            y_val[ :-4 ] = y_val[ 4: ]
            y_val[-4] = y_val[-1] = 0
            y_val[-3] = y_val[-2] = 1

            x_val[-4] = x_val[-3] = timestamp
            x_val[-2] = x_val[-1] = timestamp + 840
        
        e.set()

threading.Thread( target=reading_func ).start()

try:
    while True:
        e.wait()
        #e.clear()

        with l:
            x_val_inst = x_val.copy()
            y_val_inst = y_val.copy()

        plt.clf()
        plt.plot( x_val_inst, y_val_inst )
        plt.draw()
        plt.pause( 0.1 )
except KeyboardInterrupt:
    plt.show()

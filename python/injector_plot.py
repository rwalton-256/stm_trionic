import numpy as np
import matplotlib.pyplot as plt

true_grams = np.array( [
    [
        0,15.42,22.11,23.36,25.72,29.09,31.94
    ],
    [
        5.48,17.51,21.76,24.84,25,28.31,30.56
    ],
    [
        0,15.46,22.24,23.93,26.72,30.82,34.35
    ],
    [
        0.17,16.23,23,25.44,27.42,30.7,33.9
    ]
] )

pulse_count = np.array( [
    2857,1904,1428,1142,952,571,258
] )

pulse_width = np.array( [
    1,1.5,2,2.5,3,5,10
] ) / 1000

# Grams per second
flow_rate_gps = true_grams / ( pulse_count * pulse_width )
# Pounds per hour
flow_rate_lbphr = flow_rate_gps * 60 * 60 * 0.00220462
# Grams per pulse
grams_per_pulse = flow_rate_gps * pulse_width

for injector in range( 4 ):
    plt.plot( pulse_width, grams_per_pulse[ injector ] )
plt.legend( [ "Inj 1", "Inj 2", "Inj 3", "Inj 4" ])
plt.grid( True )
plt.show()

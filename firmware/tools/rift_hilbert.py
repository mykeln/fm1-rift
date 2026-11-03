#!/usr/bin/env python3
"""Print the Q15 nonzero coefficients of a 31-tap Hamming Hilbert FIR.

h[n] = 2/(pi*n) for odd n, zero for even n, windowed for n=-15..15.
The center tap is zero; the real path needs a 15-sample alignment delay.
"""
import math
print([round(32768 * 2 / (math.pi * n) * (0.54 + 0.46 * math.cos(math.pi * n / 15)))
       for n in range(-15, 16, 2)])

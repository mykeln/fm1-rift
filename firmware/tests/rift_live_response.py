#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Measure preset changes after the actual UI/mixer/DAC path, with retained tails."""
import json
from pathlib import Path
import sys
import wave
import numpy as np


def main():
    root = Path(sys.argv[1]); bands = []; concentration = []
    for k in range(1, 4):
        with wave.open(str(root/f'CHORD-{k}.wav')) as w:
            fs = w.getframerate()
            x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(float).reshape(-1, 2)
        n = 32768; f = np.fft.rfftfreq(n, 1/fs)
        p = np.sum(abs(np.fft.rfft(x[-n:]*np.hanning(n)[:, None], axis=0))**2, axis=1)
        peak = int(p.argmax()); concentration.append(float(p[abs(f-f[peak]) < 4].sum()/p.sum()))
        edges = np.geomspace(25, 5000, 81)
        b = np.array([p[(f>=lo)&(f<hi)].sum() for lo, hi in zip(edges[:-1], edges[1:])])
        bands.append(b/b.sum())
    distances = [float(abs(bands[a]-bands[b]).sum()/2) for a, b in ((0, 1), (0, 2), (1, 2))]
    checks = {'all CHORD variations contain more than one strong tone': max(concentration) < .95,
              'live preset recalls change the audible spectrum': min(distances) > .1}
    report = dict(strongest_tone_fraction=concentration, pairwise_spectrum_distance=distances, checks=checks)
    print(json.dumps(report, indent=2)); (root/'live-response.json').write_text(json.dumps(report, indent=2)+'\n')
    return int(not all(checks.values()))


if __name__ == '__main__':
    sys.exit(main())

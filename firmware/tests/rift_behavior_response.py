#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Spectral assertions for behavior established by public demonstrations."""
import json
import sys
import wave
from pathlib import Path
import numpy as np


def read(path):
    with wave.open(str(path)) as f:
        fs = f.getframerate()
        x = np.frombuffer(f.readframes(f.getnframes()), dtype='<i2').reshape(-1, 2).astype(float)
    return fs, x


def power(x, fs, n=65536):
    p = np.mean([np.sum(abs(np.fft.rfft(x[b:b+n] * np.hanning(n)[:, None], axis=0))**2, axis=1)
                 for b in range(0, len(x)-n+1, n//2)], axis=0)
    return np.fft.rfftfreq(n, 1/fs), p


def main():
    root = Path(sys.argv[1]); checks = {}; report = {}
    field = {}
    for value in (0, 32, 64, 96):
        fs, x = read(root/f'field-{value}.wav')
        f, p = power(x[fs//2:], fs, 32768)
        field[value] = [float(p[abs(f-hz)<5].sum()) for hz in (100, 4000)]
    report['field_tone_power'] = field
    checks['FIELD LP passes bass and attenuates treble'] = field[32][0] > 50*field[32][1]
    checks['FIELD HP passes treble and attenuates bass'] = field[96][1] > 50*field[96][0]
    checks['FIELD center is open'] = .9 < field[64][0]/field[64][1] < 1.1
    checks['FIELD full CCW closes source'] = sum(field[0]) == 0

    spectra = []
    for k in range(3):
        fs, x = read(root/f'cluster-{k}.wav'); f, p = power(x[2*fs:6*fs], fs)
        spectra.append(p)
    # Four strongest separated pitches at each endpoint. Their midpoint
    # must retain endpoint pitches from BOTH sets, rather than glide them.
    peak_sets = []
    for k in (0, 2):
        rest = spectra[k].copy(); peaks = []
        for _ in range(4):
            at = int(rest.argmax()); peaks.append(at)
            rest[abs(f-f[at])<8] = 0
        peak_sets.append(peaks)
    retained = []
    for peaks in peak_sets:
        retained.append(sum(float(spectra[1][abs(f-f[at])<3].sum()/spectra[1].sum()) > .015 for at in peaks))
    report['cluster_endpoint_pitches_hz'] = [[float(f[at]) for at in peaks] for peaks in peak_sets]
    report['cluster_midpoint_retained_pitches'] = retained
    checks['cluster midpoint contains simultaneous pitches from both anchors'] = min(retained) >= 3
    _, returned = read(root/'cluster-return.wav'); _, endpoint = read(root/'cluster-2.wav')
    checks['SEED and SCAN return to the same procedural landscape'] = bool(np.array_equal(returned, endpoint))

    tails = {}; recordings = []
    for value in (0, 64, 127):
        fs, x = read(root/f'feed-{value}.wav'); recordings.append(x)
        tails[value] = float(20*np.log10(np.sqrt(np.mean(x[5*fs:6*fs]**2))/32768+1e-15))
    report['feed_tail_db'] = tails
    checks['FEED CCW sustains a longer tail than center'] = tails[0] > tails[64]+20
    checks['FEED CW sustains a longer tail than center'] = tails[127] > tails[64]+20
    checks['unshifted and shifted paths align at neutral CELL'] = bool(np.array_equal(recordings[0], recordings[2]))
    routes = []
    for k in (0, 1):
        fs, x = read(root/f'route-{k}.wav'); f, p = power(x[2*fs:6*fs], fs)
        edges = np.geomspace(20, 8000, 65)
        bands = np.array([p[(f>=a)&(f<b)].sum() for a, b in zip(edges[:-1], edges[1:])])
        routes.append(bands/bands.sum())
    distance = float(abs(routes[0]-routes[1]).sum()/2)
    report['feed_routing_spectrum_distance'] = distance
    checks['FEED sides use distinct shifting routes'] = distance > .15
    report['checks'] = checks
    print(json.dumps(report, indent=2))
    (root/'behavior-response.json').write_text(json.dumps(report, indent=2)+'\n')
    return int(not all(checks.values()))


if __name__ == '__main__':
    sys.exit(main())

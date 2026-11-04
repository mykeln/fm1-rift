#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Long-note acceptance probes; listening remains a separate requirement.

Six seconds of the factory CAVERN drone, followed by thirty seconds of release.
The default large-space patch should still have a useful tail twenty seconds
later, decay rather than stick, and retain stereo information.
"""
import json
import sys
import wave
from pathlib import Path
import numpy as np


def main():
    root = Path(sys.argv[1])
    with wave.open(str(root / 'CAVERN-base.wav')) as f:
        fs = f.getframerate()
        x = np.frombuffer(f.readframes(f.getnframes()), dtype='<i2').astype(float).reshape(-1, 2) / 32768
    def rms_db(start, end):
        a = x[int(start * fs):int(end * fs)]
        return float(20 * np.log10(np.sqrt(np.mean(a * a)) + 1e-15))
    early, late, end = rms_db(7, 8), rms_db(26, 27), rms_db(34, 35)
    stereo = float(np.corrcoef(x[8 * fs:10 * fs].T)[0, 1])
    with wave.open(str(root / 'CAVERN-switch.wav')) as f:
        switched = np.frombuffer(f.readframes(f.getnframes()), dtype='<i2').astype(float).reshape(-1, 2) / 32768
    switch_db = float(20 * np.log10(np.sqrt(np.mean(switched[7 * fs:8 * fs] ** 2)) + 1e-15))
    # Stationary-note timbral motion: compare normalized spectral energy in
    # broad bands, rather than sample differences caused by oscillator phase.
    with wave.open(str(root / 'CAVERN-evolving.wav')) as f:
        moving = np.frombuffer(f.readframes(f.getnframes()), dtype='<i2').astype(float).reshape(-1, 2)
    def spectrum(start):
        a = moving[start * fs:(start + 4) * fs]
        n = 8192
        freq = np.fft.rfftfreq(n, 1 / fs)
        power = np.mean([np.sum(abs(np.fft.rfft(a[b:b+n] * np.hanning(n)[:, None], axis=0)) ** 2, axis=1)
                         for b in range(0, len(a)-n+1, n//2)], axis=0)
        edges = np.geomspace(30, 8000, 49)
        bands = np.array([power[(freq >= lo) & (freq < hi)].sum() for lo, hi in zip(edges[:-1], edges[1:])])
        return bands / max(bands.sum(), 1e-30)
    motion = float(np.abs(spectrum(8) - spectrum(40)).sum() / 2)
    checks = {
        'changing algorithm preserves the existing tail': switch_db > early - 6,
        'held note develops timbral motion': motion > .1,
        'audible decay twenty seconds after release': late > -60,
        'decay loses energy rather than sticking': end < early - 6,
        'tail retains stereo spread': abs(stereo) < .85,
        'no clipping': bool(np.max(np.abs(x)) < .95),
    }
    report = dict(early_tail_db=early, twenty_second_tail_db=late, final_tail_db=end,
                  stereo_correlation=stereo, switched_tail_db=switch_db, timbral_motion=motion, checks=checks)
    print(json.dumps(report, indent=2))
    (root / 'space-response.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if all(checks.values()) else 1


if __name__ == '__main__':
    sys.exit(main())

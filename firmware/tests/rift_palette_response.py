#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Guard tonal definition and sustained levels; these are listening proxies.

Peak concentration is energy within +/- 3 Hz of the strongest 16 separated
spectral peaks. Diffuse broadband rumble scores lower than a pitched drone.
Do not loudness-normalize the renders: buildup and bank balance must be visible.
"""
import json
import sys
import wave
from pathlib import Path
import numpy as np

# 0.7 intentionally changes macro semantics and all factory patches. Assess
# sound/level properties rather than comparing hashes against the old DSP.


def db(x):
    return float(20 * np.log10(np.sqrt(np.mean(x * x)) + 1e-15))


def features(path):
    with wave.open(str(path)) as f:
        fs = f.getframerate()
        x = np.frombuffer(f.readframes(f.getnframes()), dtype='<i2').astype(float).reshape(-1, 2) / 32768
    a = x[12*fs:16*fs]
    n = 32768
    freq = np.fft.rfftfreq(n, 1/fs)
    p = np.mean([np.sum(abs(np.fft.rfft(a[b:b+n] * np.hanning(n)[:, None], axis=0))**2, axis=1)
                 for b in range(0, len(a)-n+1, n//2)], axis=0)
    remaining = p.copy()
    captured = 0
    for _ in range(16):
        peak = int(remaining.argmax())
        lo, hi = max(0, peak-2), peak+3
        captured += remaining[lo:hi].sum()
        remaining[lo:hi] = 0
    return dict(rms_db=db(a), peak_db=float(20*np.log10(abs(x).max()+1e-15)),
                buildup_db=db(a)-db(x[2*fs:4*fs]),
                peak_concentration=float(captured/p.sum()),
                treble_fraction=float(p[freq>3000].sum()/p.sum()),
                sub_fraction=float(p[freq<35].sum()/p.sum()),
                tail_db=db(x[18*fs:19*fs]),
                stereo_correlation=float(np.corrcoef(a.T)[0, 1]))


def spectrum(path):
    with wave.open(str(path)) as f:
        fs = f.getframerate()
        x = np.frombuffer(f.readframes(f.getnframes()), dtype='<i2').astype(float).reshape(-1, 2) / 32768
    n = 8192
    freq = np.fft.rfftfreq(n, 1/fs)
    p = np.mean([np.sum(abs(np.fft.rfft(x[b:b+n] * np.hanning(n)[:, None], axis=0))**2, axis=1)
                 for b in range(0, len(x)-n+1, n//2)], axis=0)
    edges = np.geomspace(20, 16000, 129)
    bands = np.array([p[(freq>=lo)&(freq<hi)].sum() for lo, hi in zip(edges[:-1], edges[1:])])
    return bands/max(bands.sum(), 1e-30), db(x)


def main():
    root = Path(sys.argv[1])
    report = {p.stem: features(p) for p in sorted(root.glob('*.wav'))}
    for name, f in report.items():
        print(f"{name:10} {f['rms_db']:6.1f}dB  buildup={f['buildup_db']:5.1f}dB  "
              f"tonal={f['peak_concentration']:.2f}  treble={f['treble_fraction']:.3f}  sub={f['sub_fraction']:.3f}")
    checks = {'all 27 factory sounds rendered': len(report) == 27}
    for name, f in report.items():
        checks[f'{name} retains pitched definition'] = f['peak_concentration'] > .8
        checks[f'{name} avoids subsonic dominance'] = f['sub_fraction'] < .08
        checks[f'{name} avoids excessive treble'] = f['treble_fraction'] < .01
        checks[f'{name} sustained level'] = -26 < f['rms_db'] < -12
        checks[f'{name} bounded peaks'] = f['peak_db'] < -1
        checks[f'{name} bounded buildup'] = f['buildup_db'] < 6
    for mode in {n.rsplit('-', 1)[0] for n in report}:
        levels = [report[f'{mode}-{v}']['rms_db'] for v in (1, 2, 3)]
        checks[f'{mode} bank balance within 6 dB'] = max(levels)-min(levels) < 6
    # Inspect actual broad spectral response across every mode and every dial,
    # rather than relying on different sample hashes or white-box state.
    controls = {}
    for lo in sorted((root/'controls').glob('*-0.wav')):
        a, ad = spectrum(lo)
        b, bd = spectrum(lo.with_name(lo.name.replace('-0.wav', '-1.wav')))
        distance = float(abs(a-b).sum()/2)
        name = lo.stem[:-2]
        controls[name] = dict(spectrum_distance=distance, level_change_db=abs(ad-bd))
        checks[f'{name} audible dial response'] = distance > .015 or abs(ad-bd) > .5
    checks['all 13 dials measured in all 9 modes'] = len(controls) == 117
    if (root/'controls/matched-CLUSTER.wav').exists():
        a, _ = spectrum(root/'controls/matched-CLUSTER.wav')
        b, _ = spectrum(root/'controls/matched-WAVES.wav')
        checks['CLUSTER differs spectrally from WAVES at identical macros'] = float(abs(a-b).sum()/2) > .25
    failures = [name for name, ok in checks.items() if not ok]
    result = dict(sounds=report, controls=controls, checks=checks)
    (root/'palette-response.json').write_text(json.dumps(result, indent=2)+'\n')
    for failure in failures:
        print(f'FAIL: {failure}')
    print(f'Palette: {len(checks)} checks, {len(failures)} failures')
    return int(bool(failures))


if __name__ == '__main__':
    sys.exit(main())

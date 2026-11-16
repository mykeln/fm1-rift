#!/usr/bin/env python3
"""Check live preset recalls and audio deadlines; restore parameters. Never flash or save."""
import argparse
import json
import time
from pathlib import Path
import mido
import serial
from serial.tools import list_ports

HDR = [125, 70, 76]
ROOT = Path(__file__).resolve().parents[1]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--expect-version', default='RIFT 0.8')
    ap.add_argument('--all-presets', action='store_true')
    ap.add_argument('--output', type=Path, default=ROOT/'firmware/build/rift-hardware-health.json')
    args = ap.parse_args()
    ports = [p.device for p in list_ports.comports() if p.product == 'Felucca']
    if len(ports) != 1:
        raise RuntimeError('Expected exactly one Felucca serial interface')
    results = []
    with serial.Serial(ports[0], 115200, timeout=.03) as console:
        def status():
            console.reset_input_buffer()
            console.write(b'\nstatus\n')
            end = time.monotonic()+.4
            data = bytearray()
            while time.monotonic() < end:
                data.extend(console.read(4096))
            raw = data.decode(errors='replace')
            if 'felucca '+args.expect_version not in raw:
                raise RuntimeError('Unexpected firmware; no patch changed: '+raw)
            values = {}
            for line in raw.replace('\\r', '').replace('\\n', '\n').splitlines():
                pair = line.strip().split()
                if len(pair) == 2 and pair[1].isdigit():
                    values[pair[0]] = int(pair[1])
            if 'audio_late' not in values:
                raise RuntimeError('Incomplete status response: '+raw)
            return values
        initial = status()
        with mido.open_input('Felucca') as inp, mido.open_output('Felucca') as out:
            def req(cmd, payload):
                for _ in inp.iter_pending():
                    pass
                for _ in range(4):
                    out.send(mido.Message('sysex', data=HDR+[cmd]+payload))
                    end = time.monotonic()+1
                    while time.monotonic() < end:
                        for msg in inp.iter_pending():
                            if msg.type != 'sysex' or list(msg.data[:4]) != HDR+[cmd]:
                                continue
                            body = list(msg.data[4:])
                            if cmd == 31 and body[:2] != payload[:2]:
                                continue
                            return body
                        time.sleep(.002)
                raise TimeoutError('No matching reply for command '+str(cmd))
            def params(body):
                return [(body[k] | body[k+1] << 7)-8192 for k in range(3, len(body), 2)]
            selected = req(27, [])[0]
            saved = req(29, [0])
            original = params(saved)
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.with_suffix('.before.json').write_text(json.dumps(
                dict(selected=selected, engine=saved[1], preset=saved[2], params=original), indent=2)+'\n')
            restored = False
            try:
                req(27, [0])
                for pi in (range(27) if args.all_presets else (4, 13, 22)):
                    req(8, [15, pi])
                    time.sleep(.3)
                    state = req(29, [0])
                    a = status()
                    time.sleep(.5)
                    b = status()
                    result = dict(preset=pi, mode=params(state)[106],
                                  preset_recalled=state[1] == 15 and state[2] == pi and params(state)[106] == pi % 9,
                                  cpu_pct=b['cpu_pct'], audio_max_us=b['audio_max_us'],
                                  elapsed_ms=b['uptime_ms']-a['uptime_ms'],
                                  missed_deadlines=b['audio_late']-a['audio_late'],
                                  master_q12=b.get('master_q12'), usb_level_fixed=b.get('usb_level_fixed'))
                    results.append(result)
                    print(json.dumps(result), flush=True)
            finally:
                req(8, [saved[1], saved[2]])
                # Restore only fields changed by recall. Writing untouched
                # legacy/hidden values through editor descriptors can clamp them.
                for attempt in range(3):
                    back = req(29, [0])
                    differences = [(i, value) for i, value in enumerate(original) if params(back)[i] != value]
                    if not differences:
                        break
                    for i, value in differences:
                        u = value+8192
                        req(31, [0, i, u & 127, u >> 7 & 127])
                back = req(29, [0])
                restored = params(back) == original and back[1:3] == saved[1:3]
                req(27, [selected])
                print('Original parameters restored and verified' if restored else 'RESTORE FAILED', flush=True)
                args.output.parent.mkdir(parents=True, exist_ok=True)
                args.output.write_text(json.dumps(dict(version=args.expect_version, initial=initial,
                                                      results=results, parameters_restored=restored,
                                                      restore_differences=[dict(parameter=i, expected=v, actual=params(back)[i])
                                                                           for i, v in enumerate(original) if params(back)[i] != v]), indent=2)+'\n')
                if not restored:
                    raise RuntimeError('Live parameters did not fully restore')
    failures = [r for r in results if not r['preset_recalled'] or r['missed_deadlines'] or r['cpu_pct'] > 80]
    if failures:
        raise SystemExit(f'FAIL: {len(failures)} presets failed recall or the real-time budget; see {args.output}')
    print(f'Hardware: {len(results)} presets recalled, zero missed deadlines, audio CPU <=80%')


if __name__ == '__main__':
    main()

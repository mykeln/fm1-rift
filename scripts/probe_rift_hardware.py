#!/usr/bin/env python3
"""Capture only the Felucca USB input; restore the live patch in finally. No flash/update commands."""
import argparse
import json
import subprocess
import re
import time
from pathlib import Path
import mido
import serial
from serial.tools import list_ports

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "firmware/build/audio-diagnosis/hardware"
HDR = [125,70,76]

def status():
    ports = [p.device for p in list_ports.comports() if p.product == "Felucca"]
    if len(ports) != 1: raise RuntimeError("Expected exactly one Felucca serial interface")
    with serial.Serial(ports[0], 115200, timeout=.1) as s:
        s.write(b"\nstatus\n")
        end = time.monotonic() + 2
        data = bytearray()
        while time.monotonic() < end: data.extend(s.read(4096))
    raw = data.decode("utf-8", errors="replace")
    counters = {}
    for line in raw.splitlines():
        pair = line.split()
        if len(pair) == 2 and pair[1].isdigit(): counters[pair[0]] = int(pair[1])
    return raw, counters


def main():
    global OUT
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--expect-version", default="RIFT 0.6")
    ap.add_argument("--space", action="store_true", help="48 seconds of an unchanged CAVERN drone, then 32 seconds of tail")
    ap.add_argument("--hold-autosave", action="store_true", help="diagnostic: sample-info reads defer autosave without changing firmware or preferences")
    args = ap.parse_args()
    raw, initial_status = status()
    if f"felucca {args.expect_version}" not in raw.splitlines():
        raise RuntimeError(f"Expected {args.expect_version}; device status was:\n{raw}")
    if args.space: OUT = ROOT / ("firmware/build/audio-space/hardware-hold" if args.hold_autosave else "firmware/build/audio-space/hardware")
    OUT.mkdir(parents=True,exist_ok=True)
    listing=subprocess.run(["ffmpeg","-hide_banner","-f","avfoundation","-list_devices","true","-i",""],
                           capture_output=True,text=True,check=False).stderr
    audio=listing.split("AVFoundation audio devices:",1)[-1]
    device=re.search(r'\[(\d+)\] Felucca\s*$',audio,re.M)
    if not device: raise RuntimeError("No exact Felucca audio input; refusing to record another source")
    with mido.open_input("Felucca") as inp, mido.open_output("Felucca") as out:
        time.sleep(.3)
        def req(cmd,args):
            for _ in inp.iter_pending(): pass
            # Opening a fresh CoreMIDI port can lose its first request.
            # These read/select/parameter operations are idempotent; retry
            # them, and match parameter IDs so a late ACK cannot satisfy the
            # following parameter write.
            for attempt in range(3):
                out.send(mido.Message("sysex",data=HDR+[cmd]+args))
                end=time.monotonic()+1
                while time.monotonic()<end:
                    for msg in inp.iter_pending():
                        if msg.type != "sysex" or list(msg.data[:4]) != HDR+[cmd]: continue
                        payload = list(msg.data[4:])
                        if cmd == 31 and payload[:2] != args[:2]: continue
                        if cmd == 27 and args and payload[:1] != args[:1]: continue
                        return payload
                    time.sleep(.002)
            raise TimeoutError(f"No reply after retries: {cmd}")
        def param(i,v):
            u=v+8192
            req(31,[0,i,u&127,(u>>7)&127])
        selected = req(27, [])[0]
        saved=req(29,[0])
        original=[(saved[k]|saved[k+1]<<7)-8192 for k in range(3,len(saved),2)]
        (OUT/"patch-before.json").write_text(json.dumps({"engine":saved[1],"preset":saved[2],"params":original},indent=2))
        tests=[("base",None,0),("form-0",112,0),("form-127",112,127),
               ("feed-0",113,0),("feed-127",113,127),("cell-0",35,0),("cell-127",35,127)]
        try:
            req(27, [0])
            if args.space:
                # Release the shared tank through the ordinary engine switch,
                # then start a fresh CAVERN patch without residual old tails.
                req(8, [0, 0]); time.sleep(.2)
                req(8, [15, 8]); time.sleep(.2)
                for i, v in ((34, 36), (11, 24), (9, 48), (23, 1), (40, 0)): param(i, v)
                if args.hold_autosave: req(15, [])
                _, configured_status = status()
                recording = OUT / "CAVERN-evolving.wav"
                command = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-f", "avfoundation",
                           "-i", ":" + device[1], "-t", "80", "-ar", "44100", "-ac", "2",
                           "-af", "aresample=44100,asetpts=N/SR/TB", "-c:a", "pcm_s16le", "-progress", "pipe:1", "-stats_period", "0.1", "-y", str(recording)]
                observations = {"initial": initial_status, "configured": configured_status, "autosave_held_by_sample_info_reads": args.hold_autosave}
                released = measured = False
                next_hold = 0
                with (OUT / "capture-error.log").open("w") as error:
                    child = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=error, text=True)
                    try:
                        for line in child.stdout:
                            if not line.startswith("out_time_us="): continue
                            stamp = line.split("=", 1)[1].strip()
                            if not stamp.isdigit(): continue
                            elapsed = int(stamp) / 1e6
                            if args.hold_autosave and elapsed >= next_hold:
                                req(15, []); next_hold = elapsed + 3
                            if elapsed >= 12 and not measured:
                                _, observations["driven"] = status(); measured = True
                                print("Driven status: " + json.dumps(observations["driven"]), flush=True)
                            if elapsed >= 48 and not released:
                                param(23, 0); released = True
                                observations["generator_release_output_seconds"] = elapsed
                                print(f"Generator released near output {elapsed:.2f}s", flush=True)
                        if child.wait() != 0: raise RuntimeError("Felucca audio capture failed; see capture-error.log")
                    finally:
                        if child.poll() is None: child.terminate(); child.wait()
                if not released: raise RuntimeError("Capture ended without reaching generator release")
                _, observations["after"] = status()
                observations["version"] = args.expect_version
                observations["late_block_delta"] = observations["after"]["audio_late"] - configured_status["audio_late"]
                observations["shed_voice_delta"] = observations["after"]["voices_shed"] - initial_status["voices_shed"]
                (OUT / "device-status.json").write_text(json.dumps(observations, indent=2) + "\n")
                print("Hardware space capture complete: " + str(recording), flush=True)
                return
            for mode,name in ((6,"PARTIAL"),(7,"CROSS"),(8,"CAVERN")):
                for tag,i,v in tests:
                    req(8,[0,0]); time.sleep(.2)
                    req(8,[15,mode])
                    param(34,48)  # C3 so the speaker's low-frequency rolloff cannot hide every partial.
                    if i is not None: param(i,v)
                    time.sleep(.4)
                    subprocess.run(["ffmpeg","-hide_banner","-loglevel","error","-f","avfoundation",
                        "-i",":"+device[1],"-t","2","-ar","44100","-ac","2","-c:a","pcm_s16le","-y",str(OUT/f"{name}-{tag}.wav")],check=True)
                    print(f"captured {name}-{tag}",flush=True)
        finally:
            req(27, [0])
            req(8,[saved[1],saved[2]])
            for i,v in enumerate(original): param(i,v)
            back=req(29,[0])
            restored=[(back[k]|back[k+1]<<7)-8192 for k in range(3,len(back),2)]
            assert restored==original,"Live patch did not fully restore"
            req(27, [selected])
            print("Restored and verified all original live parameters and selected track.",flush=True)

if __name__=="__main__": main()

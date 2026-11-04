#!/usr/bin/env python3
"""Measure sustained-tone spectra and post-gate tails in probe recordings."""
import json
import sys
import wave
from pathlib import Path
import numpy as np

def read(path):
    with wave.open(str(path), "rb") as f:
        fs=f.getframerate()
        assert f.getsampwidth()==2 and f.getnchannels()==2
        a=np.frombuffer(f.readframes(f.getnframes()),dtype="<i2").astype(float).reshape(-1,2)/32768
    return fs,a

def feature(path):
    fs,a=read(path)
    start,end=(2*fs,4*fs) if len(a)>5*fs else (len(a)//2,len(a))
    x=a[start:end]; n=8192
    powers=[]
    for b in range(0,len(x)-n+1,n//2):
        powers.append(np.sum(abs(np.fft.rfft(x[b:b+n]*np.hanning(n)[:,None],axis=0))**2,axis=1))
    p=np.mean(powers,axis=0)
    freq=np.fft.rfftfreq(n,1/fs)
    edges=np.geomspace(30,10000,81)
    bands=np.array([p[(freq>=lo)&(freq<hi)].sum() for lo,hi in zip(edges[:-1],edges[1:])])
    bands/=max(bands.sum(),1e-30)
    result={"rms_db":float(20*np.log10(np.sqrt(np.mean(x*x))+1e-15)),
            "centroid_hz":float((p*freq).sum()/max(p.sum(),1e-30)),
            "above_300hz":float(p[freq>300].sum()/max(p.sum(),1e-30))}
    if len(a)>5*fs:
        result["tail_db"]=float(20*np.log10(np.sqrt(np.mean(a[5*fs:6*fs]**2))+1e-15))
    return result,bands

def main():
    root=Path(sys.argv[1]); data={}; bands={}; fail=0
    for path in sorted(root.glob("*.wav")):
        if path.stem=="current": continue
        data[path.stem],bands[path.stem]=feature(path)
    for mode in ("PARTIAL","CROSS","CAVERN"):
        print(f"{mode}: base {json.dumps(data[mode+'-base'])}")
        for control in ("time","form","feed","cell"):
            lo=f"{mode}-{control}-0"; hi=f"{mode}-{control}-127"
            if lo not in data: continue
            tv=float(np.abs(bands[lo]-bands[hi]).sum()/2)
            db=abs(data[lo]["rms_db"]-data[hi]["rms_db"])
            print(f"  {control}: spectrum-distance={tv:.3f}, level-change={db:.2f}dB"+
                  (f", tails={data[lo]['tail_db']:.1f}/{data[hi]['tail_db']:.1f}dB" if "tail_db" in data[lo] else ""))
            if control=="form":
                ok=tv>=.15 or db>=3
                print(f"  FORM broad audible response: {'ok' if ok else 'FAIL'}"); fail+=not ok
            if control=="cell":
                ok=tv>=.2
                print(f"  CELL broad audible response: {'ok' if ok else 'FAIL'}"); fail+=not ok
            if control=="feed" and "tail_db" in data[lo]:
                ok=data[hi]["tail_db"]-data[lo]["tail_db"]>=12
                print(f"  FEED extends the audible tail: {'ok' if ok else 'FAIL'}"); fail+=not ok
            if control=="time" and "tail_db" in data[lo]:
                ok=data[hi]["tail_db"]-data[lo]["tail_db"]>=6
                print(f"  TIME changes the processor decay timing: {'ok' if ok else 'FAIL'}"); fail+=not ok
        if mode=="PARTIAL":
            ok=data[mode+"-base"]["above_300hz"]>=.10
            print(f"  PARTIAL has audible upper partials: {'ok' if ok else 'FAIL'}"); fail+=not ok
    for a,b in (("PARTIAL","CROSS"),("PARTIAL","CAVERN"),("CROSS","CAVERN")):
        tv=float(np.abs(bands[a+'-base']-bands[b+'-base']).sum()/2)
        print(f"{a}/{b} spectrum-distance={tv:.3f}")
    (root/"response.json").write_text(json.dumps(data,indent=2)+"\n")
    return int(bool(fail))

if __name__=="__main__": sys.exit(main())

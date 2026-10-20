"""Exercise the actual Windows C++ frame reader against a separate shared-memory producer."""
import mmap,struct,subprocess,time,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
stride=3840*2160*12
with mmap.mmap(-1,4096+3*stride,tagname=r'Local\MinecraftifyAnythingFrame') as view:
    import os
    struct.pack_into('<IIIIQIIqiI',view,0,0x5450434d,1,4096,3,stride,3840,2160,0,-1,os.getpid())
    probe=subprocess.Popen([str(ROOT/'local/build/Release/frame_probe.exe'),'2'],stdout=subprocess.PIPE,text=True)
    for frame in range(1,80):
        slot=frame%3;desc=256+slot*128
        struct.pack_into('<Q',view,desc,frame*2+1)
        struct.pack_into('<QQIIfffI',view,desc+8,frame,frame*10,64,64,.05,2048,70,7)
        struct.pack_into('<dddfffIqq',view,desc+48,1,65.62,2,0,0,0,1,time.monotonic_ns(),time.monotonic_ns())
        view[4096+slot*stride:4096+slot*stride+64*64*12]=bytes([frame%255])*(64*64*12)
        struct.pack_into('<Q',view,desc,frame*2+2);struct.pack_into('<i',view,40,slot);struct.pack_into('<q',view,32,frame)
        time.sleep(.02)
    output=probe.communicate(timeout=5)[0];assert probe.returncode==0,output
    reports=[json.loads(line) for line in output.splitlines()];assert reports[-1]['frames']>10,output
    assert reports[0]['bytes']==64*64*12 and reports[0]['hostFrame']%10==0
    print(json.dumps({'passed':True,'frames':reports[-1]['frames'],'checks':['native mapping connection','coherent three-layer snapshots','descriptor/frame identity']},indent=2))

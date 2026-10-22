"""Install Rory's approved local recordings into isolated mod sound overrides only.

Purchased/generated audio is never distributed with source. Run after generating
and approving the local previews. Requires no network access or paid services.
"""
from pathlib import Path
import argparse, hashlib, json, re, struct, wave

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'local/voice-patch'

def chunks(data):
    assert data[:4]==b'RIFF' and data[8:12]==b'WAVE'
    pos=12
    while pos+8<=len(data):
        tag,n=struct.unpack_from('<4sI',data,pos)
        assert pos+8+n<=len(data)
        yield tag,data[pos+8:pos+8+n]
        pos+=8+n+n%2

def facial_metadata(data,name,time_range=(1.80,2.68)):
    text=data.decode('cp1252')
    # Keep scene timing and other words; replace removed titles and old mouth cues.
    text=re.sub(r'\bDoctor\s+Freeman\b','Steve',text)
    text=text.replace('Freeman','Steve')
    text=re.sub(r'WORD Doctor [\d.]+ [\d.]+\s*\{[^}]*\}\s*','',text)
    def word(match):
        punctuation=match[1];a=float(match[2]);z=float(match[3])
        if not match[4].strip():return '' # obsolete duplicate placeholder words
        if name=='gman_exit01.wav':a,z=time_range
        if name=='gman_exit07.wav':a,z=5.75,6.71
        times=[a,a+(z-a)*.22,a+(z-a)*.36,a+(z-a)*.84,z]
        phones='\n'.join(f'{code} {phone} {times[i]:.3f} {times[i+1]:.3f} 1.000'
                          for i,(code,phone) in enumerate([(115,'s'),(116,'t'),(105,'iy'),(118,'v')]))
        return f'WORD Steve{punctuation} {a:.3f} {z:.3f}\n{{\n{phones}\n}}'
    text=re.sub(r'WORD Steve([.,?!]?) ([\d.]+) ([\d.]+)\s*\{([^}]*)\}',word,text)
    assert 'Freeman' not in text and 'Doctor' not in text
    return text.encode('cp1252')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--time-take',choices=['take18','whisper2'],default='whisper2')
    args=parser.parse_args()
    time_file='Time-Steve-whisper2-original-Time.wav' if args.time_take=='whisper2' else 'Time-Steve-take18-no-stutter.wav'
    time_range=(1.39,2.14) if args.time_take=='whisper2' else (1.80,2.68)
    entries=[]
    for game in ('hl1','hl2'):
        for item in json.loads((BASE/f'edits-{game}-v2.json').read_text()):
            rel=Path(item['file'])
            source=BASE/'generated'/rel
            if rel.name=='gman_exit01.wav':source=BASE/'previews'/time_file
            assert source.is_file(),source
            entries.append((game,rel,source))
    report=[]
    # Prepare every output before installing any file.
    prepared=[]
    for game,rel,source in entries:
        runtime='runtime-halflife' if game=='hl1' else 'runtime-halflife2-v2'
        mod=(ROOT/'local'/runtime/'minecraftify').resolve()
        target=mod/rel.relative_to(game)
        assert target.resolve().is_relative_to(mod),target
        data=source.read_bytes()
        items=list(chunks(data))
        if game=='hl2':
            original=dict(chunks((BASE/'source'/rel).read_bytes()))
            metadata=facial_metadata(original[b'VDAT'],rel.name,time_range)
            items=[(tag,value) for tag,value in items if tag!=b'VDAT']+[(b'VDAT',metadata)]
        body=b'WAVE'+b''.join(tag+struct.pack('<I',len(value))+value+(b'\0' if len(value)%2 else b'') for tag,value in items)
        output=b'RIFF'+struct.pack('<I',len(body))+body
        staged=BASE/'approved-install'/rel
        staged.parent.mkdir(exist_ok=True,parents=True);staged.write_bytes(output)
        with wave.open(str(staged)) as w:
            assert w.getnchannels()==1 and w.getsampwidth() in (1,2)
            assert w.getnframes()>0
            duration=w.getnframes()/w.getframerate()
        prepared.append((staged,target,mod,rel,duration))
    for staged,target,mod,rel,duration in prepared:
        target.parent.mkdir(exist_ok=True,parents=True)
        assert target.resolve().is_relative_to(mod)
        if target.exists():
            backup=BASE/'before-approved-install'/rel
            backup.parent.mkdir(exist_ok=True,parents=True)
            if not backup.exists():backup.write_bytes(target.read_bytes())
        target.write_bytes(staged.read_bytes())
        digest=hashlib.sha256(target.read_bytes()).hexdigest()
        assert digest==hashlib.sha256(staged.read_bytes()).hexdigest()
        report.append(dict(file=str(rel),target=str(target),duration=duration,sha256=digest))
    (BASE/'installed-voices.json').write_text(json.dumps(dict(installed=True,paid_services_used=False,
        ending=args.time_take,files=report),indent=2))
    print(f'Installed and verified {len(report)} approved WAV overrides in the isolated mods.')

if __name__=='__main__':main()

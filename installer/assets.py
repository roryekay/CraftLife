"""Owned-file readers and local voice patch recipes. No full recordings are shipped.

MS ADPCM decoding follows libsndfile 1.2.2 ms_adpcm.c (Erik de Castro Lopo,
1999-2017), LGPL-2.1-or-later. This module is LGPL-2.1-or-later; see licenses.
"""
from pathlib import Path
import base64,hashlib,struct,zlib

def sha(data):return hashlib.sha256(data).hexdigest()

def chunks(data):
    if data[:4]!=b'RIFF' or data[8:12]!=b'WAVE':raise ValueError('Invalid owned WAV')
    p=12
    while p+8<=len(data):
        tag,n=struct.unpack_from('<4sI',data,p)
        if p+8+n>len(data):raise ValueError('Truncated WAV')
        yield tag,data[p+8:p+8+n];p+=8+n+n%2

def normalize(data):
    items=list(chunks(data));fields=dict(items);fmt=fields[b'fmt ']
    encoding,channels,rate,_,align,bits=struct.unpack_from('<HHIIHH',fmt)
    if encoding==1:return data
    if encoding!=2 or channels!=1 or not 7<=align<=4096:raise ValueError('Unsupported owned WAV format')
    count=struct.unpack_from('<H',fmt,20)[0]
    coefficients=[struct.unpack_from('<hh',fmt,22+4*i) for i in range(count)]
    adaptation=(230,230,230,230,307,409,512,614,768,614,512,409,307,230,230,230)
    pcm=bytearray();encoded=fields[b'data']
    for pos in range(0,len(encoded),align):
        block=encoded[pos:pos+align]
        if len(block)<7:raise ValueError('Truncated ADPCM block')
        predictor,delta,s1,s0=struct.unpack_from('<BHhh',block)
        if predictor>=len(coefficients):raise ValueError('Invalid ADPCM predictor')
        a,b=coefficients[predictor];pcm.extend(struct.pack('<hh',s0,s1))
        for byte in block[7:]:
            for nibble in (byte>>4,byte&15):
                value=max(-32768,min(32767,((s1*a+s0*b)>>8)+(nibble-16 if nibble&8 else nibble)*delta))
                delta=max(16,(delta*adaptation[nibble])>>8);s0,s1=s1,value
                pcm.extend(struct.pack('<h',value))
    if b'fact' in fields:pcm=pcm[:struct.unpack_from('<I',fields[b'fact'])[0]*2]
    fmt=struct.pack('<HHIIHH',1,1,rate,rate*2,2,16)
    items=[(tag,fmt if tag==b'fmt ' else bytes(pcm) if tag==b'data' else value) for tag,value in items if tag!=b'fact']
    body=b'WAVE'+b''.join(tag+struct.pack('<I',len(v))+v+b'\0'*(len(v)%2) for tag,v in items)
    return b'RIFF'+struct.pack('<I',len(body))+body

def vpk_member(index,member):
    """Read one bounded member from Valve VPK v1/v2, without extracting paths."""
    index=Path(index)
    with index.open('rb') as f:
        header=f.read(12);magic,version,size=struct.unpack('<III',header)
        if magic!=0x55aa1234 or version not in (1,2) or size>64*1024*1024:raise ValueError('Unsupported owned VPK')
        header_size=28 if version==2 else 12
        f.seek(header_size);tree=f.read(size)
    p=0
    def string():
        nonlocal p
        end=tree.index(0,p);s=tree[p:end].decode('utf-8');p=end+1;return s
    while True:
        ext=string()
        if not ext:break
        while True:
            folder=string()
            if not folder:break
            while True:
                name=string()
                if not name:break
                crc,preload,archive,offset,length,end=struct.unpack_from('<IHHIIH',tree,p);p+=18
                if end!=0xffff or length>32*1024*1024:raise ValueError('Invalid VPK entry')
                prefix=tree[p:p+preload];p+=preload
                full=(('' if folder==' ' else folder+'/')+name+'.'+ext).lower()
                if full!=member.lower():continue
                source=index if archive==0x7fff else index.with_name(index.name.replace('_dir.vpk',f'_{archive:03}.vpk'))
                with source.open('rb') as f:
                    f.seek(offset+(header_size+size if archive==0x7fff else 0));data=prefix+f.read(length)
                if len(data)!=preload+length or zlib.crc32(data)&0xffffffff!=crc:raise ValueError('Owned VPK member checksum failed')
                return data
    raise ValueError('Missing owned voice: '+member)

def owned_voice(game,relative):
    loose=Path(game)/('valve' if relative.startswith('sound/gman/') else 'hl2')/relative
    if loose.is_file():return loose.read_bytes()
    return vpk_member(Path(game)/'hl2/hl2_sound_vo_english_dir.vpk',relative)

def make_patch(original,target):
    base=normalize(original)
    # Fixed anchors locate unchanged audio even when a new phrase shifts samples.
    anchors={}
    for i in range(0,len(base)-63,32):anchors.setdefault(base[i:i+64],i)
    ops=[];pending=bytearray();p=0
    while p<len(target):
        at=anchors.get(target[p:p+64])
        if at is None:pending.append(target[p]);p+=1;continue
        if pending:ops.append(['data',base64.b64encode(zlib.compress(bytes(pending),9)).decode()]);pending.clear()
        n=64
        while at+n<len(base) and p+n<len(target) and base[at+n]==target[p+n]:n+=1
        ops.append(['copy',at,n]);p+=n
    if pending:ops.append(['data',base64.b64encode(zlib.compress(bytes(pending),9)).decode()])
    return {'source_sha256':sha(original),'base_sha256':sha(base),'target_sha256':sha(target),'size':len(target),'ops':ops}

def apply_patch(original,recipe):
    if sha(original)!=recipe['source_sha256']:raise ValueError('Owned voice version differs; Steam English files are required')
    base=normalize(original)
    if sha(base)!=recipe['base_sha256']:raise ValueError('Voice decode checksum failed')
    out=bytearray()
    for op in recipe['ops']:
        if op[0]=='copy':
            _,p,n=op
            if p<0 or n<0 or p+n>len(base):raise ValueError('Invalid voice copy span')
            out.extend(base[p:p+n])
        elif op[0]=='data':out.extend(zlib.decompress(base64.b64decode(op[1])))
        else:raise ValueError('Invalid voice patch operation')
        if len(out)>32*1024*1024:raise ValueError('Oversized voice patch')
    if len(out)!=recipe['size'] or sha(out)!=recipe['target_sha256']:raise ValueError('Patched voice checksum failed')
    return bytes(out)

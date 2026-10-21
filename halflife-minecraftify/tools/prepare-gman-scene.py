"""Prepare a local owned-map variant which enters HL2's actual ending choreography.

No BSP is distributed. Geometry, lighting and packed assets remain byte-for-byte
in place; only the entity lump and its header descriptor change in the mod copy.
"""
from pathlib import Path
import hashlib,io,json,re,shutil,struct,zipfile

ROOT=Path(__file__).resolve().parents[1]
SOURCE=Path('C:/Program Files (x86)/Steam/steamapps/common/Half-Life 2/hl2/maps/d3_breen_01.bsp')
MOD=ROOT/'local/runtime-halflife2-v2/minecraftify'
data=SOURCE.read_bytes()
assert data[:4]==b'VBSP' and struct.unpack_from('<i',data,4)[0] in (19,20,21)
offset,size=struct.unpack_from('<ii',data,8)
entities=data[offset:offset+size].rstrip(b'\0').decode('latin1')
for name in ('template_gman','lcs_gman_out','teleport_player_gman_1'):
    assert f'"targetname" "{name}"' in entities,name
blocks=re.findall(r'\{[^{}]*\}',entities)
for i,block in enumerate(blocks):
    if '"classname" "logic_auto"' in block:
        # Prevent unrelated chapter-opening schedules from competing with the ending.
        block=re.sub(r'"globalstate"\s*"[^"]*"\s*','',block)
        blocks[i]=block[:-1]+'"globalstate" "craftlife_skip_normal_intro"\n}'
outputs=[('template_gman','Spawn','',.1),('teleport_player_gman_1','Teleport','',.2),
         ('mic_gman','Enable','',.3),('weaponstrip_end_game','Strip','',.2),
         ('lcs_gman_out','Start','',.8)]
cue='\n{\n"classname" "logic_auto"\n"spawnflags" "1"\n"targetname" "craftlife_gman_ending"\n'
cue+=''.join(f'"OnMapSpawn" "{target},{action},{value},{delay},1"\n' for target,action,value,delay in outputs)+'}\n'
replacement=('\n'.join(blocks)+cue+'\0').encode('latin1')
patched=bytearray(data)
patched.extend(b'\0'*((-len(patched))%16))
new_offset=len(patched);patched.extend(replacement)
struct.pack_into('<ii',patched,8,new_offset,len(replacement))
maps=MOD/'maps';maps.mkdir(exist_ok=True)
target=maps/'mc_gman_ending.bsp';target.write_bytes(patched)
# The renamed map asks for reflection textures under its own name. Copy only
# those owned, embedded map materials into the matching isolated mod directory.
pak_offset,pak_size=struct.unpack_from('<ii',data,8+40*16)
reflection_files=0
if pak_size:
    with zipfile.ZipFile(io.BytesIO(data[pak_offset:pak_offset+pak_size])) as archive:
        prefix='materials/maps/d3_breen_01/'
        for name in archive.namelist():
            if not name.lower().startswith(prefix) or not name.lower().endswith(('.vtf','.vmt')):continue
            rel=Path('materials/maps/mc_gman_ending')/name[len(prefix):]
            dest=MOD/rel
            assert dest.resolve().is_relative_to(MOD.resolve())
            assert archive.getinfo(name).file_size<32*1024*1024
            dest.parent.mkdir(exist_ok=True,parents=True);dest.write_bytes(archive.read(name))
            reflection_files+=1
for extra in ('graphs/d3_breen_01.ain','d3_breen_01.nav'):
    source=SOURCE.parent/extra
    if source.is_file():
        dest=maps/extra.replace('d3_breen_01','mc_gman_ending');dest.parent.mkdir(exist_ok=True,parents=True);shutil.copyfile(source,dest)
assert patched[16:len(data)]==data[16:]
report=dict(map='mc_gman_ending',source_sha256=hashlib.sha256(data).hexdigest(),
            installed_sha256=hashlib.sha256(patched).hexdigest(),original_game_unchanged=True,
            changed_lumps=['entities'],owned_assets_local_only=True,reflection_files=reflection_files)
(ROOT/'local/voice-patch/prepared-ending-map.json').write_text(json.dumps(report,indent=2))
print('Prepared isolated mc_gman_ending map; original BSP unchanged.')

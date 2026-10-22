"""Prepare a local owned-map variant which enters HL2's actual ending choreography.

No BSP is distributed. Geometry, lighting and packed assets remain byte-for-byte
in place; only the entity lump and its header descriptor change in the mod copy.
"""
from pathlib import Path
import argparse,hashlib,io,json,re,shutil,struct,zipfile

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source',type=Path,default=Path('C:/Program Files (x86)/Steam/steamapps/common/Half-Life 2/hl2/maps/d3_breen_01.bsp'))
parser.add_argument('--mod',type=Path,default=ROOT/'local/runtime-halflife2-v2/minecraftify')
args=parser.parse_args()
SOURCE=args.source
MOD=args.mod
MOD.mkdir(parents=True,exist_ok=True)
(ROOT/'local/voice-patch').mkdir(parents=True,exist_ok=True)
data=SOURCE.read_bytes()
assert data[:4]==b'VBSP' and struct.unpack_from('<i',data,4)[0] in (19,20,21)
offset,size=struct.unpack_from('<ii',data,8)
entities=data[offset:offset+size].rstrip(b'\0').decode('latin1')
for name in ('template_gman','lcs_gman_out','teleport_player_gman_1'):
    assert f'"targetname" "{name}"' in entities,name
blocks=re.findall(r'\{[^{}]*\}',entities)
marker='\n{\n"classname" "info_target"\n"targetname" "craftlife_overworld_marker"\n"origin" "0 0 0"\n}\n'
for i,block in enumerate(blocks):
    if '"targetname" "scene_change_3"' in block:
        # Finish the authored transition, then use one actual camera for both
        # the original actor and Minecraft instead of script_intro's two views.
        block=re.sub(r'"OnTrigger"\s*"intro_outro_fx_1,[^"]*"\s*',lambda m:'' if any(v in m.group() for v in (',26.45,',',27.5,',',27.6,',',28.1,',',28.11,')) else m.group(),block)
        blocks[i]=block[:-1]+'"OnTrigger" "craftlife_overworld_marker,AddOutput,origin 1 0 0,9.1,1"\n"OnTrigger" "intro_outro_fx_1,Deactivate,,9.1,1"\n"OnTrigger" "starfield,TurnOff,,9.1,1"\n"OnTrigger" "train_starfield,Stop,,9.1,1"\n}'
    elif '"targetname" "view_gman_end_1"' in block:
        blocks[i]=re.sub(r'"wait"\s*"[^"]*"','"wait" "-1"',block)
    elif '"targetname" "lcs_gman_out"' in block:
        blocks[i]=block[:-1]+'"OnStart" "craftlife_overworld_marker,AddOutput,origin 0 -1 0,0,1"\n}'
    elif '"targetname" "Thedoor"' in block:
        # Handoff follows the real door closing; hold credits until Minecraft saves.
        block=re.sub(r'"OnFullyClosed"\s*"logic_ending_credits,[^"]*"\s*','',block)
        blocks[i]=block[:-1]+'"OnFullyClosed" "craftlife_overworld_marker,AddOutput,origin 2 0 0,0,1"\n"OnOpen" "craftlife_overworld_marker,AddOutput,origin 1 1 0,0,1"\n}'
# The full campaign gets the same event markers, retaining its original startup.
campaign=('\n'.join(blocks)+marker+'\0').encode('latin1')
campaign_data=bytearray(data);campaign_data.extend(b'\0'*((-len(data))%16))
campaign_offset=len(campaign_data);campaign_data.extend(campaign)
struct.pack_into('<ii',campaign_data,8,campaign_offset,len(campaign))
maps=MOD/'maps';maps.mkdir(exist_ok=True)
(maps/'d3_breen_01.bsp').write_bytes(campaign_data)
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
replacement=('\n'.join(blocks)+cue+marker+'\0').encode('latin1')
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

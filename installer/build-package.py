"""Build a source-allowlisted private Windows player package from tested outputs."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,sys,urllib.request,zipfile
sys.path.insert(0,str(Path(__file__).resolve().parent))
from assets import make_patch,apply_patch,owned_voice,sha
SOURCE=Path(__file__).resolve().parents[1];WORK=SOURCE.parent
PORT=WORK/'halflife-minecraftify';VERSION='0.3.0-test.2'
OUT=Path(os.environ.get('CRAFTLIFE_PACKAGE_DIR',str(PORT/'local/packages'/('CraftLife-'+VERSION+'-windows-x64'))))
PYTHON=Path(sys.executable).parent
NODE=Path(os.environ.get('CRAFTLIFE_NODE',str(Path.home()/'AppData/Local/hermes/node/node.exe')))

def put(source,name):
    dest=OUT/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest)
def text(name,data):
    p=OUT/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(data,encoding='utf-8')
def main():
    if OUT.exists():raise SystemExit('Output exists; choose a new version or move the previous package aside')
    OUT.mkdir(parents=True)
    for name in ('setup.py','assets.py'):put(SOURCE/'installer'/name,'installer/'+name)
    put(PORT/'tools/start-native.mjs','halflife-minecraftify/tools/start-native.mjs')
    put(PORT/'tools/prepare-gman-scene.py','halflife-minecraftify/tools/prepare-gman-scene.py')
    for name in ('relay.mjs','travel-catalog.mjs','moments.json'):put(PORT/'adapters/common'/name,'halflife-minecraftify/adapters/common/'+name)
    put(NODE,'tools/node/node.exe');put(NODE.parent/'LICENSE','licenses/Node.txt')
    for name in ('python.exe','python312.dll','python3.dll','vcruntime140.dll','vcruntime140_1.dll'):put(PYTHON/name,'tools/python/'+name)
    put(PYTHON/'LICENSE.txt','licenses/Python.txt')
    for p in (PYTHON/'DLLs').glob('*'):
        if p.suffix.lower() in ('.dll','.pyd') and not p.name.startswith(('_test','_tkinter','tcl','tk')):put(p,'tools/python/DLLs/'+p.name)
    with zipfile.ZipFile(OUT/'tools/python/python312.zip','w',zipfile.ZIP_DEFLATED) as z:
        for p in sorted((PYTHON/'Lib').rglob('*.py')):
            parts=p.relative_to(PYTHON/'Lib').parts
            if any(x in parts for x in ('site-packages','__pycache__','test','tests','idlelib','tkinter','turtledemo','ensurepip','venv')):continue
            z.write(p,Path(*parts).as_posix())
    text('tools/python/python312._pth','python312.zip\n.\nDLLs\n../../installer\n')
    for name,action in [('Install.cmd','install'),('Play Half-Life 1.cmd','play halflife'),('Play Half-Life 2.cmd','play halflife2'),('Play G-Man Ending.cmd','play halflife2 --ending'),('Verify.cmd','verify')]:
        text(name,'@echo off\n"%~dp0tools\\python\\python.exe" "%~dp0installer\\setup.py" '+action+' %*\nif errorlevel 1 echo Setup or launch did not complete. Read the message above.\npause\n')
    put(PORT/'guest/build/libs/minecraftify-guest-0.1.0.jar','guest/mods/minecraftify-guest-0.1.0.jar')
    api=PORT/'local/minecraft-halflife2/mods/fabric-api-0.161.0+26.3.jar'
    assert sha(api.read_bytes())=='86f16178a3cecc887a85a4cfe9a79d92fa7341d8f39b5951a4d6ad800ab657a6'
    put(api,'guest/mods/'+api.name)
    text('guest/options.txt','onboardAccessibility:false\npauseOnLostFocus:false\nrenderDistance:6\nsimulationDistance:6\nmaxFps:60\nfullscreen:false\nguiScale:2\ntutorialStep:none\nrenderClouds:"false"\nsoundCategory_master:0.3\n')
    put(SOURCE/'installer/fabric-version.json','fabric-version.json')
    with urllib.request.urlopen('https://piston-meta.mojang.com/mc/game/version_manifest_v2.json',timeout=45) as f:metadata=json.load(f)
    entry=next(x for x in metadata['versions'] if x['id']=='26.3')
    assert entry['url'].startswith('https://piston-meta.mojang.com/')
    with urllib.request.urlopen(entry['url'],timeout=45) as f:vanilla=f.read()
    assert hashlib.sha1(vanilla).hexdigest()==entry['sha1']
    (OUT/'vanilla-version.json').write_bytes(vanilla)
    runtime='payload/halflife-minecraftify/local/'
    for name in ('hl.dll','client.dll'):
        sub='dlls' if name=='hl.dll' else 'cl_dlls'
        put(PORT/'local/hl1-build/Release'/name,runtime+'runtime-halflife/minecraftify/'+sub+'/'+name)
    put(WORK/'halflife2-minecraftify/local/native-build/Release/MinecraftifyHL2.dll',runtime+'runtime-halflife2-v2/minecraftify/addons/MinecraftifyHL2.dll')
    for name in ('block.dx80.vtx','block.dx90.vtx','block.mdl','block.phy','block.sw.vtx','block.vvd'):
        put(PORT/'local/runtime-halflife2-v2/minecraftify/models/minecraftify'/name,runtime+'runtime-halflife2-v2/minecraftify/models/minecraftify/'+name)
    cfg=(PORT/'tools/prepare-valve-runtime.ps1').read_text().split("$cfg=@'\n",1)[1].split("\n'@",1)[0]
    for game in ('halflife','halflife2'):
        name='runtime-halflife' if game=='halflife' else 'runtime-halflife2-v2'
        suffix='/minecraftify/'+('minecraftify.cfg' if game=='halflife' else 'cfg/minecraftify.cfg')
        extra='\nm_rawinput 1\ncl_lw 0\nsv_maxspeed 640\ncl_forwardspeed 640\ncl_backspeed 640\ncl_sidespeed 640\n' if game=='halflife' else '\nr_drawviewmodel 0\n'
        text(runtime+name+suffix,cfg+extra)
    for directory in ('runtime-halflife2-v2','runtime-halflife2-v2/bin'):
        base=runtime+directory+'/'
        put(PORT/'local/reshade-runtime/ReShade32.dll',base+'d3d9.dll')
        put(PORT/'local/renderer32/Release/MinecraftifyRenderer.addon32',base+'MinecraftifyRenderer.addon32')
        for name in ('ReShade.ini','MinecraftifyPreset.ini'):put(PORT/'local/runtime-halflife2-v2/bin'/name,base+name)
        put(PORT/'shaders/Minecraftify.fx',base+'reshade-shaders/Shaders/Minecraftify.fx')
        put(PORT/'third_party/ReShade.fxh',base+'reshade-shaders/Shaders/ReShade.fxh')
    recipes=[];voice_report=read_json(PORT/'local/voice-patch/installed-voices.json')
    for item in voice_report['files']:
        rel=Path(item['file']);game='halflife' if rel.parts[0]=='hl1' else 'halflife2';member=Path(*rel.parts[1:]).as_posix()
        original=(PORT/'local/voice-patch/source'/rel).read_bytes();target=Path(item['target']).read_bytes()
        assert sha(target)==item['sha256']
        recipe=make_patch(original,target);recipe.update(game=game,member=member)
        assert apply_patch(original,recipe)==target
        # Read the actual owned loose/VPK source as a second provenance check.
        owned=Path('C:/Program Files (x86)/Steam/steamapps/common')/('Half-Life' if game=='halflife' else 'Half-Life 2')
        assert owned_voice(owned,member)==original
        name='voice-patches/'+game+'-'+rel.name+'.json';text(name,json.dumps(recipe,separators=(',',':')));recipes.append(name)
        literal=sum(len(__import__('base64').b64decode(op[1])) for op in recipe['ops'] if op[0]=='data')
        print('Voice recipe',rel.name,len(target),'target bytes;',literal,'compressed edit bytes')
    for folder in ('halflife-minecraftify','halflife2-minecraftify'):
        for p in (WORK/folder/'licenses').glob('*.txt'):put(p,'licenses/'+folder+'-'+p.name)
    put(SOURCE/'installer/LICENSE-LGPL-2.1.txt','licenses/LGPL-2.1.txt')
    for name in ('INSTALL.md','THIRD_PARTY.md','REUSE_REVIEW.md','TEST_RESULTS.md'):put(SOURCE/'installer'/name,name)
    games={}
    for game,folder,exe in [('halflife','Half-Life','hl.exe'),('halflife2','Half-Life 2','hl2.exe')]:
        p=Path('C:/Program Files (x86)/Steam/steamapps/common')/folder/exe
        games[game]={'exe_sha256':sha(p.read_bytes())}
    manifest={'version':VERSION,'games':games,'ending_map_sha256':sha(Path('C:/Program Files (x86)/Steam/steamapps/common/Half-Life 2/hl2/maps/d3_breen_01.bsp').read_bytes()),'voice_recipes':recipes,'files':{}}
    for p in sorted(OUT.rglob('*')):
        if p.is_file():manifest['files'][p.relative_to(OUT).as_posix()]=sha(p.read_bytes())
    text('manifest.json',json.dumps(manifest,indent=2)+'\n')
    archive=OUT.parent/(OUT.name+'.zip')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for p in sorted(OUT.rglob('*')):
            if p.is_file():z.write(p,p.relative_to(OUT).as_posix())
    checksum=sha(archive.read_bytes());archive.with_suffix('.zip.sha256').write_text(checksum+'  '+archive.name+'\n')
    print(json.dumps({'archive':str(archive),'sha256':checksum,'bytes':archive.stat().st_size,'files':len(manifest['files'])},indent=2))
def read_json(p):return json.loads(p.read_text(encoding='utf-8'))
if __name__=='__main__':main()

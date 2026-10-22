"""Run the shipped installer/runtime against isolated profile/world fixtures."""
from pathlib import Path
import hashlib,json,subprocess,uuid
import setup as installer
ROOT=Path(__file__).resolve().parents[1];WORK=ROOT.parent;PORT=WORK/'halflife-minecraftify'
package=PORT/'local/packages/CraftLife-0.3.0-test.2-windows-x64'
fixture=Path.home()/'Games'/('CraftLife Setup Test '+uuid.uuid4().hex[:8]);fixture.mkdir(parents=True)
minecraft=fixture/'Minecraft with spaces';minecraft.mkdir();target=fixture/'CraftLife with spaces'
original={'profiles':{'existing':{'name':'Keep this profile','gameDir':str(fixture/'existing world')}},'settings':{'keep':True},'clientToken':'FIXTURE-NOT-AN-ACCOUNT-TOKEN'}
(minecraft/'launcher_profiles.json').write_text(json.dumps(original))
python=package/'tools/python/python.exe';setup=package/'installer/setup.py'
games=Path('C:/Program Files (x86)/Steam/steamapps/common')
args=['install','--install-dir',str(target),'--minecraft-dir',str(minecraft),'--halflife',str(games/'Half-Life'),'--halflife2',str(games/'Half-Life 2')]
checks=[]
def run(arguments,expected=0):
    p=subprocess.run([str(python),str(setup),*arguments],capture_output=True,text=True,timeout=180)
    assert p.returncode==expected,(p.returncode,p.stdout,p.stderr)
    return p.stdout+p.stderr
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
run(['verify']);checks.append('portable packaged Python and SHA manifest')
run(args);data=json.loads((minecraft/'launcher_profiles.json').read_text())
assert data['profiles']['existing']==original['profiles']['existing'] and data['settings']==original['settings'] and data['clientToken']==original['clientToken']
for game in ('halflife','halflife2'):
    assert data['profiles']['craftlife-'+game]['gameDir']==str(target/'halflife-minecraftify/local'/('minecraft-'+game))
assert (minecraft/'versions/craftlife-fabric-0.19.5-26.3/craftlife-fabric-0.19.5-26.3.json').is_file()
checks+=['fresh install with spaces and both owned Steam games','unrelated profiles and settings preserved','isolated official-Launcher Fabric profiles']
report=json.loads((PORT/'local/voice-patch/installed-voices.json').read_text())
for voice in report['files']:
    rel=Path(voice['file']);runtime='runtime-halflife' if rel.parts[0]=='hl1' else 'runtime-halflife2-v2'
    dest=target/'halflife-minecraftify/local'/runtime/'minecraftify'/Path(*rel.parts[1:])
    assert sha(dest)==voice['sha256'],str(rel)
checks.append('all ten voice recipes reproduce approved WAVs byte-for-byte')
for name in ('d3_breen_01.bsp','mc_gman_ending.bsp'):
    rel=Path('local/runtime-halflife2-v2/minecraftify/maps')/name
    assert sha(target/'halflife-minecraftify'/rel)==sha(PORT/rel)
checks.append('campaign and shortcut ending maps match tested outputs')
world=target/'halflife-minecraftify/local/minecraft-halflife2/saves/fixture';world.mkdir(parents=True);(world/'keep.txt').write_text('keep world')
options=target/'halflife-minecraftify/local/minecraft-halflife2/options.txt';options.write_text('custom player options')
ini=target/'halflife-minecraftify/local/runtime-halflife2-v2/bin/ReShade.ini';ini.write_text('[GENERAL]\nPerformanceMode=1\n')
run(args)
assert options.read_text()=='custom player options' and ini.read_text()=='[GENERAL]\nPerformanceMode=1\n' and (world/'keep.txt').read_text()=='keep world'
assert json.loads((minecraft/'launcher_profiles.json').read_text())['profiles']['existing']==original['profiles']['existing']
checks.append('repeat install preserves worlds/options/ReShade/profile')
bad=fixture/'unsupported';bad.mkdir();(bad/'hl.exe').write_bytes(b'wrong executable')
bad_args=list(args);bad_args[bad_args.index('--halflife')+1]=str(bad);bad_args[bad_args.index('--install-dir')+1]=str(fixture/'must-not-exist')
assert 'Unsupported Half-Life' in run(bad_args,1);assert not (fixture/'must-not-exist').exists();checks.append('unsupported game refused before writes')
foreign=fixture/'foreign';foreign.mkdir();(foreign/'keep.txt').write_text('preserve');foreign_args=list(args);foreign_args[foreign_args.index('--install-dir')+1]=str(foreign)
assert 'empty folder' in run(foreign_args,1);assert (foreign/'keep.txt').read_text()=='preserve';checks.append('unrelated nonempty install directory refused')
jar=target/'halflife-minecraftify/local/minecraft-halflife2/mods/minecraftify-guest-0.1.0.jar';saved=jar.read_bytes();jar.write_bytes(b'tampered')
assert 'checksum failed' in run(['play','halflife2','--install-dir',str(target)],1);jar.write_bytes(saved);checks.append('modified installed guest refused before launch')
backups=sorted((target/'backups').glob('launcher-profiles-*.json'));assert len(backups)==2 and json.loads(backups[0].read_text())==original
checks.append('exact original launcher settings backup')
for only,title,other in [('halflife','Half-Life','halflife2'),('halflife2','Half-Life 2','halflife')]:
    sandbox=fixture/('HL1 only' if only=='halflife' else 'HL2 only');sandbox.mkdir()
    steam=sandbox/'Steam';common=steam/'steamapps/common';common.mkdir(parents=True)
    installer.junction(common/title,games/title)
    mc=sandbox/'Minecraft';mc.mkdir();(mc/'launcher_profiles.json').write_text(json.dumps(original))
    dest=sandbox/'Game';single=['install','--steam-dir',str(steam),'--install-dir',str(dest),'--minecraft-dir',str(mc)]
    run(single);receipt=json.loads((dest/'installation.json').read_text());assert set(receipt['games'])=={only}
    profiles=json.loads((mc/'launcher_profiles.json').read_text())['profiles']
    assert 'craftlife-'+only in profiles and 'craftlife-'+other not in profiles
    assert '-Dminecraftify.games='+only+' ' in profiles['craftlife-'+only]['javaArgs']
    absent='runtime-halflife' if other=='halflife' else 'runtime-halflife2-v2'
    assert not (dest/'halflife-minecraftify/local'/absent).exists()
    for voice in report['files']:
        rel=Path(voice['file']);game='halflife' if rel.parts[0]=='hl1' else 'halflife2'
        if game!=only:continue
        native='runtime-halflife' if only=='halflife' else 'runtime-halflife2-v2'
        assert sha(dest/'halflife-minecraftify/local'/native/'minecraftify'/Path(*rel.parts[1:]))==voice['sha256']
    assert 'not installed' in run(['play',other,'--install-dir',str(dest)],1)
    checks.append(title+'-only automatic detection, profiles, voices, payload and missing-game refusal')
    keep=dest/'halflife-minecraftify/local'/('minecraft-'+only)/'saves/keep.txt';keep.parent.mkdir(parents=True);keep.write_text('existing progress')
    installer.junction(common/('Half-Life 2' if other=='halflife2' else 'Half-Life'),games/('Half-Life 2' if other=='halflife2' else 'Half-Life'))
    run(single);receipt=json.loads((dest/'installation.json').read_text());assert set(receipt['games'])=={'halflife','halflife2'}
    profiles=json.loads((mc/'launcher_profiles.json').read_text())['profiles']
    assert all('-Dminecraftify.games=halflife,halflife2 ' in profiles['craftlife-'+g]['javaArgs'] for g in ('halflife','halflife2'))
    assert keep.read_text()=='existing progress';checks.append('add second game to '+title+' install without losing progress')
empty=fixture/'Empty Steam';(empty/'steamapps/common').mkdir(parents=True)
assert 'No Half-Life game found' in run(['install','--steam-dir',str(empty),'--install-dir',str(fixture/'No games'),'--minecraft-dir',str(minecraft)],1)
assert not (fixture/'No games').exists();checks.append('no supported games found refused before install writes')
result={'passed':True,'checks':checks,'fixture':str(fixture),'install':str(target)}
(PORT/'local/installer-validation.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

"""Install two isolated profiles into the existing authenticated official launcher.

Does not read launcher account files or copy saves/credentials. Back up only profile
settings, preserving every other top-level launcher setting in its original file.
Close Minecraft Launcher before running, then reopen it to select a profile.
"""
from pathlib import Path
import argparse,datetime,json,os,shutil,hashlib
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--minecraft-root',type=Path,default=Path(os.environ['APPDATA'])/'.minecraft')
args=p.parse_args()
profiles=args.minecraft_root/'launcher_profiles.json'
version='fabric-loader-0.19.5-26.3'
if not (args.minecraft_root/'versions'/version/(version+'.json')).is_file():raise SystemExit('The reviewed Fabric 0.19.5 / Minecraft 26.3 launcher version must already be installed.')
jar=ROOT/'guest/build/libs/minecraftify-guest-0.1.0.jar'
apis=list((ROOT/'local/gradle/caches/modules-2/files-2.1/net.fabricmc.fabric-api/fabric-api/0.161.0+26.3').glob('*/*.jar'))
if not jar.is_file() or len(apis)!=1:raise SystemExit('Build the guest and pinned Fabric API dependencies first.')
data=json.loads(profiles.read_text(encoding='utf-8'))
stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
backup=ROOT/'local'/('launcher-profiles-before-'+stamp+'.json')
backup.write_text(json.dumps(data.get('profiles',{}),indent=2),encoding='utf-8')
for game,title in [('halflife','Minecraftify Half-Life'),('halflife2','Minecraftify Half-Life 2')]:
    instance=ROOT/'local'/('minecraft-'+game);mods=instance/'mods';mods.mkdir(parents=True,exist_ok=True)
    for source in (jar,apis[0]):shutil.copy2(source,mods/source.name)
    if not (instance/'options.txt').exists():
        (instance/'options.txt').write_text('onboardAccessibility:false\npauseOnLostFocus:false\nrenderDistance:6\nsimulationDistance:6\nmaxFps:60\nfullscreen:false\nguiScale:2\ntutorialStep:none\nrenderClouds:"false"\n')
    key='minecraftify-anything-'+game
    data.setdefault('profiles',{})[key]={
      'name':title+' (development)','type':'custom','lastVersionId':version,
      'gameDir':str(instance),'resolution':{'width':1280,'height':720},
      'javaArgs':f'-Xmx3G --enable-native-access=ALL-UNNAMED -Dminecraftify.autoWorld=true -Dminecraftify.game={game} -Dminecraftify.world=minecraftify-valve-{game}'
    }
    manifest={source.name:hashlib.sha256(source.read_bytes()).hexdigest() for source in (jar,apis[0])}
    (instance/'minecraftify-installed.json').write_text(json.dumps({'profile':key,'files':manifest},indent=2))
temp=profiles.with_suffix('.minecraftify.tmp');temp.write_text(json.dumps(data,indent=2),encoding='utf-8');temp.replace(profiles)
print('Installed two development profiles; no account files or existing worlds were copied.')
print('Profile-settings backup:',backup)

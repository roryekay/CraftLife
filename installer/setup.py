"""User-level CraftLife installer. Official Launcher owns authentication/downloads."""
from pathlib import Path
import argparse,datetime,hashlib,json,os,re,shutil,socket,subprocess,sys,uuid
from assets import apply_patch,owned_voice,sha

ROOT=Path(__file__).resolve().parents[1]
DEFAULT=Path.home()/'Games/CraftLife'
VERSION='craftlife-fabric-0.19.5-26.3'
GAMES={'halflife':('Half-Life','hl.exe','runtime-halflife'),'halflife2':('Half-Life 2','hl2.exe','runtime-halflife2-v2')}

def read(p):return json.loads(Path(p).read_text(encoding='utf-8-sig'))
def write(p,data):
    p=Path(p);p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
def inside(root,relative):
    relative=Path(relative)
    if relative.is_absolute() or '..' in relative.parts:raise ValueError('Unsafe package path')
    result=(root/relative).resolve()
    if not result.is_relative_to(root.resolve()):raise ValueError('Package path escapes its folder')
    return result
def copy(source,target,preserve=False):
    if preserve and target.exists():return
    target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
def digest(p):
    with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def package():
    manifest=read(ROOT/'manifest.json')
    for rel,value in manifest['files'].items():
        p=inside(ROOT,rel)
        if not p.is_file() or digest(p)!=value:raise ValueError('Package checksum failed: '+rel)
    return manifest
def running(name):
    result=subprocess.run(['tasklist.exe','/FI','IMAGENAME eq '+name,'/FO','CSV','/NH'],capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW,check=True)
    return any(line.lower().startswith('"'+name.lower()+'"') for line in result.stdout.splitlines())
def find_game(game):
    import winreg
    libraries=[]
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER,r'Software\Valve\Steam') as key:libraries.append(Path(winreg.QueryValueEx(key,'SteamPath')[0]))
    except OSError:pass
    libraries.append(Path(os.environ.get('ProgramFiles(x86)','C:/Program Files (x86)'))/'Steam')
    for steam in list(libraries):
        vdf=steam/'steamapps/libraryfolders.vdf'
        if vdf.is_file():libraries += [Path(p.replace('\\\\','\\')) for p in re.findall(r'"path"\s*"([^"]+)"',vdf.read_text())]
    title,exe,_=GAMES[game]
    for library in libraries:
        folder=library/'steamapps/common'/title
        if (folder/exe).is_file():return folder
    value=input('Enter the folder containing '+exe+' for '+title+': ').strip().strip('"')
    if not value:raise ValueError('Game selection cancelled')
    return Path(value)
def require_game(game,folder,manifest):
    exe=folder/GAMES[game][1]
    if not exe.is_file() or digest(exe)!=manifest['games'][game]['exe_sha256']:raise ValueError('Unsupported '+GAMES[game][0]+' executable. This build requires the reviewed Steam version; original files were preserved.')
    if game=='halflife2':
        bsp=folder/'hl2/maps/d3_breen_01.bsp'
        if not bsp.is_file() or digest(bsp)!=manifest['ending_map_sha256']:raise ValueError('Unsupported HL2 ending map; original files were preserved.')
def junction(link,target):
    if link.exists():
        if link.resolve()!=target.resolve():raise ValueError('Existing asset mount points elsewhere: '+str(link))
        return
    if not target.is_dir():return
    env=os.environ.copy();env['CRAFTLIFE_LINK']=str(link);env['CRAFTLIFE_TARGET']=str(target)
    # Literal environment arguments; never interpolate a path into shell code.
    subprocess.run(['powershell.exe','-NoProfile','-NonInteractive','-Command',"$ErrorActionPreference='Stop'; New-Item -ItemType Junction -Path $env:CRAFTLIFE_LINK -Target $env:CRAFTLIFE_TARGET | Out-Null"],env=env,creationflags=subprocess.CREATE_NO_WINDOW,check=True)
def runtime(install,game,original):
    port=install/'halflife-minecraftify';target=port/'local'/GAMES[game][2]
    target.mkdir(parents=True,exist_ok=True)
    for p in original.iterdir():
        if p.is_file() and p.suffix.lower() in ('.exe','.dll','.asi'):copy(p,target/p.name)
    for name in (('valve','valve_hd','platform') if game=='halflife' else ('hl2','platform','steam_input')):junction(target/name,original/name)
    mod=target/'minecraftify';mod.mkdir(exist_ok=True)
    (target/'steam_appid.txt').write_text('70' if game=='halflife' else '220')
    if game=='halflife':
        (mod/'liblist.gam').write_text('game "CraftLife - Half-Life"\ngamedll "dlls/hl.dll"\nfallback_dir "valve"\ntype "singleplayer_only"\ncldll "1"\nversion "0.3.0"\n')
    else:
        engine=target/'bin'
        if engine.is_symlink() or (engine.exists() and engine.resolve()!=engine.absolute()):raise ValueError('Engine bin must be an isolated directory')
        engine.mkdir(exist_ok=True)
        for p in (original/'bin').iterdir():
            if p.is_file() and p.name.lower() not in ('d3d9.dll','reshade.ini','minecraftifypreset.ini'):copy(p,engine/p.name)
        assets=original.as_posix()
        paths=['game+mod "|gameinfo_path|."','mod+mod_write+default_write_path "|gameinfo_path|."']
        paths += ['game "'+assets+'/hl2/'+s+'.vpk"' for s in ('hl2_sound_vo_english','hl2_pak','hl2_textures','hl2_sound_misc','hl2_misc')]
        paths += ['game "'+assets+'/hl2"','gamebin "'+assets+'/hl2/bin"','platform "'+assets+'/platform/platform_misc.vpk"','platform "'+assets+'/platform"']
        (mod/'gameinfo.txt').write_text('"GameInfo"\n{\ngame "CraftLife - Half-Life 2"\ntype singleplayer_only\nFileSystem\n{\nSteamAppId 220\nSearchPaths\n{\n'+'\n'.join(paths)+'\n}\n}\n}\n')
        plugin=(mod/'addons/MinecraftifyHL2').as_posix()
        (mod/'addons').mkdir(parents=True,exist_ok=True)
        (mod/'addons/minecraftify.vdf').write_text('"Plugin" { "file" "'+plugin+'" }\n')
    return target
def destination(install,rel):
    return inside(install,rel.removeprefix('payload/'))
def install(args):
    manifest=package();target=args.install_dir.resolve();minecraft=args.minecraft_dir.resolve()
    if len(str(target))>100:raise ValueError('Choose a shorter installation folder (at most 100 characters) for the native game asset paths')
    if target==ROOT or target.is_relative_to(ROOT) or ROOT.is_relative_to(target):raise ValueError('Extract the download separately from the installation folder')
    if '"' in str(target) or '\n' in str(target):raise ValueError('Invalid installation path')
    for appdata in (os.environ.get('APPDATA'),os.environ.get('LOCALAPPDATA')):
        if appdata and target.is_relative_to(Path(appdata).resolve()):raise ValueError('Choose a Games or Documents folder outside AppData for official Launcher compatibility')
    if target.exists() and any(target.iterdir()) and not (target/'.craftlife-owned').is_file():raise ValueError('Choose an empty folder or an existing CraftLife installation')
    if any(running(exe) for _,exe,_ in GAMES.values()):raise ValueError('Close Half-Life normally before installing')
    standard_mc=(Path(os.environ['APPDATA'])/'.minecraft').resolve()
    if minecraft==standard_mc and (running('MinecraftLauncher.exe') or running('Minecraft.exe')):raise ValueError('Close Minecraft Launcher, then run Install again')
    profile=minecraft/'launcher_profiles.json'
    if not profile.is_file():raise ValueError('Open the official Minecraft Launcher once, sign in yourself, then close it before setup')
    original_profiles=profile.read_bytes();profiles=read(profile);entries=profiles.setdefault('profiles',{})
    games={game:(getattr(args,game) or find_game(game)).resolve() for game in GAMES}
    for game,folder in games.items():
        if target.is_relative_to(folder) or folder.is_relative_to(target):raise ValueError('Install CraftLife outside the original Steam game folders')
        require_game(game,folder,manifest)
        key='craftlife-'+game
        if key in entries and Path(entries[key].get('gameDir','')).resolve()!=(target/'halflife-minecraftify/local'/('minecraft-'+game)).resolve():raise ValueError('An existing CraftLife profile belongs to another install; it was preserved')
    fabric_path=minecraft/'versions'/VERSION/(VERSION+'.json');fabric=read(ROOT/'fabric-version.json');fabric['id']=VERSION
    if fabric_path.exists():
        existing=read(fabric_path)
        if {k:v for k,v in existing.items() if k not in ('time','releaseTime')}!={k:v for k,v in fabric.items() if k not in ('time','releaseTime')}:raise ValueError('Existing isolated Fabric metadata differs; it was preserved')
    # Prepare/verify every approved voice before writing the installation.
    voices=[]
    for rel in manifest['voice_recipes']:
        recipe=read(inside(ROOT,rel));game=recipe['game'];member=recipe['member']
        voices.append((game,member,apply_patch(owned_voice(games[game],member),recipe)))
    target.mkdir(parents=True,exist_ok=True);(target/'.craftlife-owned').write_text('CraftLife installation schema 1\n')
    copy(ROOT/'manifest.json',target/'manifest.json')
    for rel in manifest['files']:
        if rel.startswith('payload/'):continue
        copy(inside(ROOT,rel),destination(target,rel))
    runtimes={game:runtime(target,game,folder) for game,folder in games.items()}
    # Payload is explicit, with no original Steam/Minecraft assets or account data.
    for rel in manifest['files']:
        if not rel.startswith('payload/'):continue
        dest=destination(target,rel)
        copy(inside(ROOT,rel),dest,preserve=dest.name in ('options.txt','ReShade.ini','MinecraftifyPreset.ini'))
    for game,member,data in voices:
        dest=inside(runtimes[game]/'minecraftify',member);dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
    python=target/'tools/python/python.exe';port=target/'halflife-minecraftify'
    subprocess.run([str(python),str(port/'tools/prepare-gman-scene.py'),'--source',str(games['halflife2']/'hl2/maps/d3_breen_01.bsp'),'--mod',str(runtimes['halflife2']/'minecraftify')],check=True)
    for game in GAMES:
        instance=port/'local'/('minecraft-'+game);mods=instance/'mods'
        for p in (target/'guest/mods').iterdir():copy(p,mods/p.name)
        copy(target/'guest/options.txt',instance/'options.txt',preserve=True)
        entries['craftlife-'+game]={'name':'CraftLife - '+GAMES[game][0],'type':'custom','icon':'Grass','lastVersionId':VERSION,'gameDir':str(instance),'resolution':{'width':1280,'height':720},'javaArgs':'-Xmx3G --enable-native-access=ALL-UNNAMED -Dminecraftify.autoWorld=true -Dminecraftify.game='+game+' -Dminecraftify.world=minecraftify-valve-'+game}
    vanilla=minecraft/'versions/26.3/26.3.json'
    if not vanilla.exists():copy(ROOT/'vanilla-version.json',vanilla)
    write(fabric_path,fabric)
    if profile.read_bytes()!=original_profiles:raise ValueError('Launcher settings changed during setup. Close Launcher and retry; profiles were not overwritten')
    stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S')+'-'+uuid.uuid4().hex[:8]
    backup=target/'backups'/('launcher-profiles-'+stamp+'.json');backup.parent.mkdir(exist_ok=True);backup.write_bytes(original_profiles)
    temp=profile.with_name(profile.name+'.craftlife-'+uuid.uuid4().hex+'.tmp');write(temp,profiles);os.replace(temp,profile)
    owned={rel:digest(destination(target,rel)) for rel in manifest['files'] if rel.startswith(('payload/','guest/mods/','halflife-minecraftify/adapters/','halflife-minecraftify/tools/','tools/node/')) and not rel.endswith(('.ini','options.txt'))}
    for game in GAMES:
        executable=runtimes[game]/GAMES[game][1];owned[str(executable.relative_to(target))]=digest(executable)
        for p in (port/'local'/('minecraft-'+game)/'mods').glob('*.jar'):owned[str(p.relative_to(target))]=digest(p)
    owned.update({str((runtimes[g]/'minecraftify'/m).relative_to(target)):sha(b) for g,m,b in voices})
    for name in ('d3_breen_01.bsp','mc_gman_ending.bsp'):
        p=runtimes['halflife2']/'minecraftify/maps'/name;owned[str(p.relative_to(target))]=digest(p)
    write(target/'installation.json',{'version':manifest['version'],'games':{g:str(p) for g,p in games.items()},'minecraft':str(minecraft),'files':owned})
    (ROOT/'installation-location.txt').write_text(str(target),encoding='utf-8')
    print('\nCraftLife installed to '+str(target)+'\n\n1. Open the official Minecraft Launcher.\n2. Choose CraftLife - Half-Life or CraftLife - Half-Life 2 and Play.\n3. Wait for its isolated world, then run the matching Play button.\n\nF8 travels through both games. I: inventory. T: chat. F5: third person.\nThe G-Man ending and approved Steve voices are installed.\nExisting worlds and original Steam files were preserved.')
def installed(args):
    if args.install_dir!=DEFAULT:return args.install_dir.resolve()
    if (ROOT/'installation.json').is_file():return ROOT
    locator=ROOT/'installation-location.txt'
    return Path(locator.read_text(encoding='utf-8')).resolve() if locator.exists() else DEFAULT
def play(args):
    target=installed(args);receipt=read(target/'installation.json');manifest=read(target/'manifest.json')
    for game,folder in receipt['games'].items():require_game(game,Path(folder),manifest)
    for rel,value in receipt['files'].items():
        p=inside(target,rel.removeprefix('payload/'))
        if not p.is_file() or digest(p)!=value:raise ValueError('Installed file checksum failed: '+rel+'. Run Install again')
    try:
        with socket.create_connection(('127.0.0.1',25610),timeout=1):pass
    except OSError:raise ValueError('Start the CraftLife profile in the official Minecraft Launcher and wait for its world before pressing Play')
    command=[str(target/'tools/node/node.exe'),str(target/'halflife-minecraftify/tools/start-native.mjs'),args.game]
    if args.ending:command.append('--destination=53' if args.game=='halflife2' else '--destination=0')
    subprocess.run(command,cwd=target/'halflife-minecraftify',check=True)
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('action',choices=('install','play','verify'));parser.add_argument('game',nargs='?',choices=tuple(GAMES));parser.add_argument('--install-dir',type=Path,default=DEFAULT);parser.add_argument('--minecraft-dir',type=Path,default=Path(os.environ['APPDATA'])/'.minecraft')
    parser.add_argument('--halflife',type=Path);parser.add_argument('--halflife2',type=Path);parser.add_argument('--ending',action='store_true');args=parser.parse_args()
    try:
        if args.action=='install':install(args)
        elif args.action=='verify':package();print('Package checksums passed.')
        elif args.game:play(args)
        else:raise ValueError('Choose halflife or halflife2')
        return 0
    except (OSError,ValueError,KeyError,subprocess.SubprocessError) as error:print('CraftLife: '+str(error),file=sys.stderr);return 1
if __name__=='__main__':sys.exit(main())

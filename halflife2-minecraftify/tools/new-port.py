"""Copy only maintained source into a fresh standalone port; never import local games/worlds."""
from pathlib import Path
import argparse,json,re,shutil
ROOT=Path(__file__).resolve().parents[1]
FILES=['.gitignore','LICENSE','README.md','AGENTS.md','CLAUDE.md','CMakeLists.txt','UPSTREAM_REVIEW.md']
DIRS=['guest','native','third_party','licenses','shaders','tools','tests','docs','adapters','.agents','.claude']
EXCLUDE={'.git','.gradle','build','run','local','__pycache__'}
def create(name,destination):
    if not re.fullmatch(r'[a-z][a-z0-9-]{1,47}',name):raise ValueError('Use a lowercase slug, e.g. my-game')
    destination=Path(destination).resolve()
    if destination==ROOT or ROOT in destination.parents:raise ValueError('Destination must be outside the template checkout')
    if destination.exists() and any(destination.iterdir()):raise ValueError('Destination must be empty; existing files are preserved')
    sources=[ROOT/f for f in FILES]
    sources += [p for d in DIRS for p in (ROOT/d).rglob('*') if p.is_file() and not any(x in EXCLUDE for x in p.relative_to(ROOT).parts)]
    # Check before creating anything; missing template components are an error.
    for src in sources:
        if not src.is_file():raise FileNotFoundError(src)
    destination.mkdir(parents=True,exist_ok=True)
    for src in sources:
        dest=destination/src.relative_to(ROOT);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dest)
    adapter=destination/'adapters'/name;adapter.mkdir()
    shutil.copy2(ROOT/'adapters/stub/adapter.hpp',adapter/'adapter.hpp')
    (adapter/'port.json').write_text(json.dumps({'name':name,'status':'unimplemented','engine':None,'supportedExecutableSha256':None,'unitsPerMinecraftBlock':None,'cameraAxes':None,'graphicsApi':None,'modLoader':None,'capabilities':{'camera':False,'input':False,'collision':False,'combat':False,'health':False}},indent=2)+'\n')
    print(json.dumps({'port':name,'path':str(destination),'status':'unimplemented','sourceFiles':len(sources)},indent=2))
    return destination
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('name');parser.add_argument('destination');args=parser.parse_args();create(args.name,args.destination)

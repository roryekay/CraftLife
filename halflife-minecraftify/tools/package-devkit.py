"""Package only redistributable template source and locally built bridge artifacts."""
from pathlib import Path
import hashlib,importlib.util,json,zipfile
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('new_port',ROOT/'tools/new-port.py');port=importlib.util.module_from_spec(spec);spec.loader.exec_module(port)
def sources():
    return sorted([ROOT/f for f in port.FILES]+[p for d in port.DIRS for p in (ROOT/d).rglob('*') if p.is_file() and not any(x in port.EXCLUDE for x in p.relative_to(ROOT).parts)])
def package():
    files={p.relative_to(ROOT).as_posix():p for p in sources()}
    for name,source in [('bin/MinecraftifyRenderer.addon64','local/build/Release/MinecraftifyRenderer.addon64'),('bin/frame_probe.exe','local/build/Release/frame_probe.exe'),('bin/minecraftify-guest-0.1.0.jar','guest/build/libs/minecraftify-guest-0.1.0.jar')]:
        files[name]=ROOT/source
    assert len(files)==len(set(files))
    manifests={name:hashlib.sha256(p.read_bytes()).hexdigest() for name,p in files.items()}
    out=ROOT/'local/packages/MinecraftifyAnything-0.1.0-devkit-windows-x64.zip';out.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
        for name,p in files.items():z.write(p,name)
        z.writestr('DEVKIT.md','Developer starter, not a universal game installer. Source and skills are included. bin/ contains the built guest JAR, ReShade add-on and frame probe. The guest still needs Fabric/API and an owned Minecraft runtime; the renderer needs a reviewed compatible ReShade runtime in a host. Neither runtime nor purchased game assets are bundled. Follow README.md and docs/PORTING.md. GPU acceptance remains per-port work.\n')
        z.writestr('SHA256.json',json.dumps(manifests,indent=2)+'\n')
    digest=hashlib.sha256(out.read_bytes()).hexdigest();out.with_suffix(out.suffix+'.sha256').write_text(digest+'  '+out.name+'\n')
    print(json.dumps({'package':str(out),'bytes':out.stat().st_size,'sha256':digest,'sourceFiles':len(sources())},indent=2));return out
if __name__=='__main__':package()

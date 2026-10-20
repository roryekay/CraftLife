"""Check extraction boundaries, generated-port isolation and skill packaging."""
from pathlib import Path
import importlib.util,json,tempfile
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('new_port',ROOT/'tools/new-port.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
checks=[]
runtime=[* (ROOT/'guest/src').rglob('*'),* (ROOT/'native').rglob('*'),* (ROOT/'shaders').rglob('*')]
for p in runtime:
    if not p.is_file():continue
    text=p.read_text(encoding='utf8').lower()
    for forbidden in ['doom','eternalcraft','rdr2','scripthook','gtafire','combat_potion','boss_menu','minhook','gtastate']:
        assert forbidden not in text,(str(p.relative_to(ROOT)),forbidden)
checks.append('runtime has no inherited host-specific code/identifiers')
for base in ['.agents','.claude']:
    skill=ROOT/base/'skills/minecraftify-anything/SKILL.md'
    assert skill.is_file() and skill.read_text().startswith('---\nname: minecraftify-anything\n')
checks.append('Codex and Claude Code skills are packaged')
assert (ROOT/'.agents/skills/minecraftify-anything/SKILL.md').read_bytes()==(ROOT/'.claude/skills/minecraftify-anything/SKILL.md').read_bytes()
checks.append('both agents receive the same maintained workflow')
with tempfile.TemporaryDirectory(prefix='minecraftify-test-') as tmp:
    target=Path(tmp)/'new-port';module.create('test-game',target)
    assert not any(p.name in {'.gradle','build','run','local','.git'} for p in target.rglob('*'))
    state=json.loads((target/'adapters/test-game/port.json').read_text());assert not any(state['capabilities'].values())
    assert (target/'guest/gradle/wrapper/gradle-wrapper.jar').is_file()
    try:module.create('test-game',target)
    except ValueError:pass
    else:raise AssertionError('nonempty destination was overwritten')
    assert (target/'adapters/test-game/port.json').read_text()
checks+=['new port includes source and wrapper, excludes caches/worlds/builds','new adapter starts unimplemented','generator refuses overwriting an existing port']
print(json.dumps({'passed':True,'checks':checks},indent=2))

"""Author an original 32-inch collision cube for Source; contains no game assets."""
from pathlib import Path
ROOT=Path(__file__).resolve().parent
verts=[(-16,-16,-16),(16,-16,-16),(16,16,-16),(-16,16,-16),(-16,-16,16),(16,-16,16),(16,16,16),(-16,16,16)]
faces=[((0,3,2,1),(0,0,-1)),((4,5,6,7),(0,0,1)),((0,1,5,4),(0,-1,0)),((1,2,6,5),(1,0,0)),((2,3,7,6),(0,1,0)),((3,0,4,7),(-1,0,0))]
lines=['version 1','nodes','0 "root" -1','end','skeleton','time 0','0 0 0 0 0 0 0','end','triangles']
for face,normal in faces:
    for triangle in [(face[0],face[1],face[2]),(face[0],face[2],face[3])]:
        lines.append('mc_collision')
        for index in triangle:lines.append('0 '+' '.join(map(str,verts[index]+normal))+' 0 0')
lines+=['end'];(ROOT/'block.smd').write_text('\n'.join(lines)+'\n')
(ROOT/'block.qc').write_text('$modelname "minecraftify/block.mdl"\n$body body "block.smd"\n$surfaceprop "stone"\n$cdmaterials "minecraftify"\n$sequence idle "block.smd" fps 1\n$collisionmodel "block.smd" { $mass 1000 }\n')
print(ROOT/'block.qc')

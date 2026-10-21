"""Inspect owned map entity metadata, never copy or distribute the BSPs."""
from pathlib import Path
import json,re,struct
ROOT=Path(__file__).resolve().parents[1]
STEAM=Path('C:/Program Files (x86)/Steam/steamapps/common')
CHAPTERS={
'halflife':[('Black Mesa tram','c0a0'),('Anomalous Materials','c1a0'),('Resonance Cascade','c1a0c'),('Unforeseen Consequences','c1a1'),('Office Complex','c1a2'),("We\'ve Got Hostiles",'c1a3'),('Blast Pit','c1a4'),('Power Up','c2a1'),('On a Rail','c2a2'),('Apprehension','c2a3'),('Residue Processing','c2a4'),('Questionable Ethics','c2a4d'),('Surface Tension','c2a5'),('Forget About Freeman','c3a1'),('Lambda Core','c3a2'),('Xen','c4a1'),("Gonarch\'s Lair",'c4a2'),('Interloper','c4a1a'),('Nihilanth','c4a3')],
'halflife2':[('Point Insertion','d1_trainstation_01'),('City 17 checkpoint','d1_trainstation_02'),('A Red Letter Day','d1_trainstation_05'),('Route Kanal','d1_canals_01'),('Water Hazard','d1_canals_06'),('Black Mesa East','d1_eli_01'),("We Don\'t Go To Ravenholm",'d1_town_01'),('Highway 17','d2_coast_01'),('Sandtraps','d2_coast_07'),('Nova Prospekt','d2_prison_01'),('Entanglement','d2_prison_06'),('Anticitizen One','d3_c17_01'),('Follow Freeman','d3_c17_09'),('Our Benefactors','d3_citadel_01'),('Dark Energy','d3_breen_01')]}
BOSSES={'halflife':{'monster_tentacle':('Tentacle',1),'monster_gargantua':('Gargantua',2),'monster_bigmomma':('Gonarch',3),'monster_nihilanth':('Nihilanth',4),'monster_apache':('Apache',5)},'halflife2':{'npc_helicopter':('Hunter-Chopper',6),'npc_combinegunship':('Gunship',7),'npc_strider':('Strider',8),'npc_antlionguard':('Antlion Guard',9)}}
catalog={}
for game,folder in [('halflife','Half-Life/valve'),('halflife2','Half-Life 2/hl2')]:
    maps=STEAM/folder/'maps'
    entries=[{'label':label,'map':name,'kind':'moment','encounter':0} for label,name in CHAPTERS[game] if (maps/(name+'.bsp')).exists()]
    for path in sorted(maps.glob('*.bsp')):
        if path.stem.startswith(('background','boot','bounce','crossfire','credits')):continue
        with path.open('rb') as f:
            header=f.read(24)
            if len(header)<24:continue
            offset,size=struct.unpack_from('<ii',header,8 if game=='halflife2' else 4)
            if size<0 or size>8*1024*1024:continue
            f.seek(offset);entities=f.read(size).decode('latin1')
        types=set(re.findall(r'"classname"\s*"([^"]+)"',entities))
        for classname,(label,number) in BOSSES[game].items():
            if classname in types:entries.append({'label':f'{label} location — {path.stem}','map':path.stem,'kind':'boss','encounter':number})
    # Append to preserve existing destination indices used by launchers/tests.
    voices = [('G-Man: full ending and choice', 'c5a1', 0),
              ('G-Man: job offer', 'c5a1', 10),
              ('G-Man: accept the offer', 'c5a1', 11),
              ('G-Man: refuse the offer', 'c5a1', 12)] if game=='halflife' else [
              ('G-Man: Rise and shine', 'd1_trainstation_01', 0),
              ('G-Man: Time, Steve? / ending', 'mc_gman_ending', 0)]
    entries.extend({'label':label,'map':name,'kind':'moment','encounter':number}
                   for label,name,number in voices if (maps/(name+'.bsp')).exists() or (name=='mc_gman_ending' and (maps/'d3_breen_01.bsp').exists()))
    catalog[game]=entries
(ROOT/'adapters/common/moments.json').write_text(json.dumps(catalog,indent=2)+'\n')
resources=ROOT/'adapters/common/resources';resources.mkdir(exist_ok=True)
(resources/'minecraftify-moments.json').write_text(json.dumps(catalog,indent=2)+'\n')
lines=['#pragma once','namespace mcvalve {']
for game,entries in catalog.items():
    lines.append('inline const char* const '+game+'Maps[]={'+','.join(json.dumps(e['map']) for e in entries)+'};')
    lines.append('inline const int '+game+'Encounters[]={'+','.join(str(e['encounter']) for e in entries)+'};')
lines.append('}')
(ROOT/'adapters/common/moments.hpp').write_text('\n'.join(lines)+'\n')
print(json.dumps({k:{'moments':sum(e['kind']=='moment' for e in v),'bossLocations':sum(e['kind']=='boss' for e in v)} for k,v in catalog.items()},indent=2))

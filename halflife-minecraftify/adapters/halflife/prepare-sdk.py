"""Create a reproducible build tree from the pinned official SDK. No old port input."""
from pathlib import Path
import shutil, subprocess, xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[2]
SDK=ROOT/'local/sdk'
PIN='b1b5cf5892918535619b2937bb927e46cb097ba1'
assert subprocess.check_output(['git','-C',str(SDK),'rev-parse','HEAD'],text=True).strip()==PIN
DEST=ROOT/'local/hl1-sdk-build'
if not DEST.exists():
    shutil.copytree(SDK,DEST,ignore=shutil.ignore_patterns('.git'))
def patch(relative,old,new):
    path=DEST/relative
    text=path.read_text()
    if new in text:return
    if text.count(old)!=1:raise RuntimeError(f'patch context changed: {relative}')
    path.write_text(text.replace(old,new))
patch('dlls/client.cpp','void StartFrame( void )\n{','extern void MinecraftifyServerFrame();\nvoid StartFrame( void )\n{\n    MinecraftifyServerFrame();')
patch('dlls/client.cpp','void ClientPrecache( void )\n{','void ClientPrecache( void )\n{\n    if(g_engfuncs.pfnCheckParm("-minecraftify-tests",nullptr))UTIL_PrecacheOther("monster_zombie");')
patch('dlls/player.cpp','int CBasePlayer :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )\n{','extern float MinecraftifyPlayerDamage(float,int);\nint CBasePlayer :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )\n{\n    flDamage=MinecraftifyPlayerDamage(flDamage,bitsDamageType);\n    if(flDamage<=0)return 0;')
patch('dlls/client.cpp','cd->health\t\t\t= pev->health;','cd->health\t\t\t= pev->health;\n    cd->fuser4=pev->fuser4;')
patch('dlls/client.cpp','cd->fuser4=pev->fuser4;','cd->fuser4=pev->fuser4;\n    cd->fuser1=pev->fuser1;')
patch('dlls/client.cpp','cd->fuser1=pev->fuser1;','cd->fuser1=pev->fuser1;\n    cd->fuser2=pev->fuser2;')
patch('pm_shared/pm_shared.c','\t// Handle movement\n\tswitch ( pmove->movetype )','\tif(pmove->fuser2>50.0f && pmove->fuser2<150.0f)pmove->view_ofs[2]=pmove->fuser2-100.0f;\n\t// Handle movement\n\tswitch ( pmove->movetype )')
patch('pm_shared/pm_shared.c','\tmaxspeed = pmove->clientmaxspeed;','\tif(pmove->fuser1>=1.0f && pmove->fuser1<=2.0f){pmove->maxspeed=320.0f*pmove->fuser1;pmove->clientmaxspeed=pmove->maxspeed;}\n\tmaxspeed = pmove->clientmaxspeed;')
patch('pm_shared/pm_shared.c','\t// Decay it for simulation\n\tPM_FixupGravityVelocity();','\tif(pmove->fuser4>=1.0f && pmove->fuser4<=2.0f)pmove->velocity[2]*=pmove->fuser4;\n\t// Decay it for simulation\n\tPM_FixupGravityVelocity();')
patch('cl_dll/tri.cpp','void CL_DLLEXPORT HUD_DrawNormalTriangles( void )\n{','extern void MinecraftifyRememberProjection();\nvoid CL_DLLEXPORT HUD_DrawNormalTriangles( void )\n{\n    MinecraftifyRememberProjection();')
patch('cl_dll/inputw32.cpp','void CL_DLLEXPORT IN_DeactivateMouse (void)\n{','extern void MinecraftifyReleaseMouse();\nvoid CL_DLLEXPORT IN_DeactivateMouse (void)\n{\n    MinecraftifyReleaseMouse();')
patch('cl_dll/cdll_int.cpp','gHUD.Redraw( time, intermission );','gHUD.Redraw( time, intermission );\n    extern void MinecraftifyComposite();MinecraftifyComposite();')
patch('cl_dll/entity.cpp','int CL_DLLEXPORT HUD_AddEntity( int type, struct cl_entity_s *ent, const char *modelname )\n{','int CL_DLLEXPORT HUD_AddEntity( int type, struct cl_entity_s *ent, const char *modelname )\n{\n    extern bool MinecraftifyClientActive();\n    if(MinecraftifyClientActive() && ent==gEngfuncs.GetLocalPlayer())return 0;')
# Append the render hook at the actual end of CalcRefdef, after all camera work.
path=DEST/'cl_dll/view.cpp';text=path.read_text();signature='void CL_DLLEXPORT V_CalcRefdef( struct ref_params_s *pparams )\n{'
if 'MinecraftifyClientView(pparams)' not in text:
    start=text.index(signature);left=text.index('{',start);depth=1;n=left+1
    # This function contains comments with braces; use the next top-level function marker instead.
    end=text.index('\n}',left)
    text=text[:end]+'\n    extern void MinecraftifyClientView(ref_params_s*);\n    MinecraftifyClientView(pparams);'+text[end:]
    path.write_text(text)
path=DEST/'cl_dll/input.cpp';text=path.read_text()
sig='void CL_DLLEXPORT CL_CreateMove ( float frametime, struct usercmd_s *cmd, int active )'
if 'MinecraftifyClientMove(cmd)' not in text:
    start=text.index(sig);end=text.index('\n}',start)
    text=text[:end]+'\n    extern void MinecraftifyClientMove(usercmd_s*);\n    MinecraftifyClientMove(cmd);'+text[end:];path.write_text(text)
# Only read upstream project source lists; upstream post-build/install scripts never run.
ns={'m':'http://schemas.microsoft.com/developer/msbuild/2003'}
cmake=['cmake_minimum_required(VERSION 3.20)','project(MinecraftifyHL1 LANGUAGES C CXX)',
       'set(CMAKE_CXX_STANDARD 17)','set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)',
       'add_compile_definitions(WIN32 _WINDOWS NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)']
for project,output,adapter in [('hldll','hl','server'),('hl_cdll','client','client')]:
    proj=DEST/f'projects/vs2019/{project}.vcxproj';tree=ET.parse(proj)
    sources=[(proj.parent/e.attrib['Include'].replace('\\','/')).resolve().as_posix() for e in tree.findall('.//m:ClCompile[@Include]',ns)]
    sources.append((ROOT/f'adapters/halflife/{adapter}.cpp').as_posix())
    if output=='client':
        sources=[s for s in sources if not s.endswith('/hud_benchtrace.cpp')]
        sources.append((ROOT/'adapters/halflife/no-network-benchmark.cpp').as_posix())
        sources.append((ROOT/'adapters/halflife/compositor.cpp').as_posix())
    cmake.append(f'add_library({output} SHARED\n'+''.join(f'  "{s}"\n' for s in sources)+')')
    dirs=['dlls','engine','common','pm_shared','game_shared','public','cl_dll','cl_dll/hl','utils','external','utils/vgui/include']
    if output=='client':dirs+=['dlls/wpn_shared']
    cmake.append(f'target_include_directories({output} PRIVATE '+ ' '.join('"'+(DEST/d).as_posix()+'"' for d in dirs)+' "'+(ROOT/'native/include').as_posix()+'")')
    defs='VALVE_DLL CLIENT_WEAPONS QUIVER VOXEL QUAKE2' if output=='hl' else 'CLIENT_DLL CLIENT_WEAPONS HL_DLL'
    cmake.append(f'target_compile_definitions({output} PRIVATE {defs})')
    cmake.append(f'target_compile_options({output} PRIVATE /W0 /wd4996 /MP4)')
    cmake.append(f'target_link_libraries({output} PRIVATE ws2_32 winmm opengl32)')
    if output=='hl':cmake.append(f'target_link_options(hl PRIVATE "/DEF:{(DEST/"dlls/hl.def").as_posix()}")')
    else:
        for e in tree.findall('.//m:AdditionalDependencies',ns):print('SDK client dependencies:',e.text)
        cmake.append(f'target_link_libraries(client PRIVATE "{(DEST/"utils/vgui/lib/win32_vc16/vgui.lib").as_posix()}" "{(DEST/"lib/public/sdl2.lib").as_posix()}")')
(DEST/'CMakeLists.txt').write_text('\n'.join(cmake)+'\n')
print(DEST)

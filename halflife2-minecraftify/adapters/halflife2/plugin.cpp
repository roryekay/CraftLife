// Uses public Valve plugin/tools interfaces; no signature scanning or HalfCraft code.
#include "../../../halflife-minecraftify/adapters/common/wire.hpp"
#include "../../../halflife-minecraftify/adapters/common/movement.hpp"
#include "../../../halflife-minecraftify/adapters/common/moments.hpp"
#include <fusion/pose_channel.hpp>
#include "../../../halflife-minecraftify/adapters/common/text_input.hpp"
#include "interface.h"
#include "engine/iserverplugin.h"
#include "eiface.h"
#include "game/server/iplayerinfo.h"
#include "toolframework/itoolentity.h"
#include "engine/IEngineTrace.h"
#include "iserverunknown.h"
#include "ehandle.h"
typedef CHandle<CBaseEntity> EHANDLE;
#include "takedamageinfo.h"
#include "shareddefs.h"
#include "cdll_int.h"
#include "view_shared.h"
#include "engine/ICollideable.h"
#include "icvar.h"
#include "tier1/convar.h"
#include "tier0/icommandline.h"
#include "vgui/ISurface.h"
#include <map>
#include <tuple>
#include <cstdio>
#include <cctype>
#include <vector>

// Match the SDK's default initialization, with no dependency on game-DLL globals.
CTakeDamageInfo::CTakeDamageInfo(){
    m_vecDamageForce.Init();m_vecDamagePosition.Init();m_vecReportedPosition.Init();
    m_flDamage=m_flMaxDamage=m_flDamageBonus=0;m_flBaseDamage=BASEDAMAGE_NOT_SPECIFIED;
    m_bitsDamageType=m_iDamageCustom=m_iDamageStats=m_iDamagedOtherPlayers=m_iPlayerPenetrationCount=0;
    m_iAmmoType=-1;m_bForceFriendlyFire=false;
}
namespace {
IVEngineServer* engine=nullptr;IPlayerInfoManager* players=nullptr;IServerTools* tools=nullptr;
ICvar* cvars=nullptr;
IEngineTrace* traces=nullptr;IBaseClientDLL* client=nullptr;CGlobalVars* globals=nullptr;
mcvalve::Link link;bool paused=false;uint32_t sequence=0;char mapName[64]{};
int endingPhase=0;Vector endingAnchor;ULONGLONG endingNotice=0;
bool endingWorldHidden=false,endingPortalOpen=false,endingActorGone=false,endingCinematic=false;float endingDrawWorld=1,endingClear=0,endingSky=1,ending3dSky=1;
void restoreEndingRender(){
    if(!endingWorldHidden||!cvars)return;
    if(auto* v=cvars->FindVar("r_drawworld"))v->SetValue(endingDrawWorld);
    if(auto* v=cvars->FindVar("r_clear"))v->SetValue(endingClear);
    if(auto* v=cvars->FindVar("r_drawskybox"))v->SetValue(endingSky);
    if(auto* v=cvars->FindVar("r_3dsky"))v->SetValue(ending3dSky);
    endingWorldHidden=false;
}
void endingState(){
    if(strcmp(mapName,"mc_gman_ending")&&strcmp(mapName,"d3_breen_01"))return;
    int phase=0;Vector anchor;bool actor=false;
    for(int i=1;i<engine->GetEntityCount();i++){
        auto* e=engine->PEntityOfEntIndex(i);if(!e||e->IsFree()||!e->GetUnknown())continue;
        auto* entity=e->GetUnknown()->GetBaseEntity();char name[128]{},value[128]{};
        if(!tools->GetKeyValue(entity,"targetname",name,sizeof(name)))continue;
        if(endingPhase&&(!strcmp(name,"train_starfield")||!strcmp(name,"dynamic_light_tracktrain2"))){
            char effects[64]{};tools->GetKeyValue(entity,"effects",effects,sizeof(effects));
            if(!(atoi(effects)&EF_NODRAW)){tools->SetKeyValue(entity,"effects",float(atoi(effects)|EF_NODRAW));e->ClearTransmitState();e->StateChanged();}
        }
        if(!strcmp(name,"craftlife_overworld_marker")&&tools->GetKeyValue(entity,"origin",value,sizeof(value))){float stage=0,door=0;if(std::sscanf(value,"%f %f",&stage,&door)==2){phase=int(stage);endingPortalOpen=door>0;if(door<0)endingCinematic=true;}}
        if(!strcmp(name,"gman")&&tools->GetKeyValue(entity,"origin",value,sizeof(value))){
            actor=std::sscanf(value,"%f %f %f",&anchor.x,&anchor.y,&anchor.z)==3;
            if(actor&&endingPortalOpen&&!endingActorGone&&anchor.x>=14105&&fabsf(anchor.y-15128)<36){
                char effects[64]{};tools->GetKeyValue(entity,"effects",effects,sizeof(effects));tools->SetKeyValue(entity,"effects",float(atoi(effects)|EF_NODRAW));
                e->ClearTransmitState();e->StateChanged();endingActorGone=true;Msg("[CraftLife] G-Man crossed the portal\n");
            }
        }
    }
    if(phase>=1&&phase<=2&&(actor||phase==2)){
        if(phase!=endingPhase)Msg("[CraftLife] Overworld scene phase %d\n",phase);
        endingPhase=phase;if(actor)endingAnchor=anchor;
        if(!endingWorldHidden&&cvars){
            // The original starfield movers and emitters are no longer needed
            // once the authored transition hands scenery to Minecraft.
            std::vector<CBaseEntity*> scenery;
            for(auto* entity=tools->FirstEntity();entity;entity=tools->NextEntity(entity)){
                char name[128]{};if(!tools->GetKeyValue(entity,"targetname",name,sizeof(name)))continue;
                if(!strcmp(name,"train_starfield")||!strcmp(name,"starfield"))scenery.push_back(entity);
            }
            for(auto* entity:scenery){tools->RemoveEntity(entity);Msg("[CraftLife] Removed native starfield backdrop\n");}
            if(auto* v=cvars->FindVar("r_drawworld")){endingDrawWorld=v->GetFloat();v->SetValue(0);}
            if(auto* v=cvars->FindVar("r_clear")){endingClear=v->GetFloat();v->SetValue(1);}
            if(auto* v=cvars->FindVar("r_drawskybox")){endingSky=v->GetFloat();v->SetValue(0);}
            if(auto* v=cvars->FindVar("r_3dsky")){ending3dSky=v->GetFloat();v->SetValue(0);}
            endingWorldHidden=true;
        }
    }
    if(endingPhase&&GetTickCount64()-endingNotice>300){
        endingNotice=GetTickCount64();mcvalve::Packet p;p.kind=mcvalve::Ending;p.i[0]=endingPhase;p.i[1]=endingPortalOpen?1:0;mcvalve::toGuest(endingAnchor.Base(),p.f);link.send(p);
    }
}
IVEngineClient* engineClient=nullptr;
IClientTools* clientTools=nullptr;
fusion::PoseWriter compositePose;
bool inventory=false,mouseReleased=false,chat=false;
mcvalve::TextInput textInput;
// The current Steam client also uses SDL relative capture. IN_DeactivateMouse
// alone leaves that capture active, recentering clicks in Minecraft menus.
bool relativeOwned=false;int previousRelative=0;
vgui::ISurface* mouseSurface=nullptr;bool surfaceReleased=false;
void relativeMouse(bool release){
    if(!mouseSurface){auto m=GetModuleHandleA("vguimatsurface.dll");if(m){auto factory=reinterpret_cast<CreateInterfaceFn>(GetProcAddress(m,"CreateInterface"));if(factory)mouseSurface=static_cast<vgui::ISurface*>(factory(VGUI_SURFACE_INTERFACE_VERSION,nullptr));}}
    if(mouseSurface){
        if(release!=surfaceReleased){mouseSurface->SetCursorAlwaysVisible(release);surfaceReleased=release;}
        if(release){mouseSurface->UnlockCursor();mouseSurface->SetCursor(vgui::dc_arrow);}
    }
    auto module=GetModuleHandleA("SDL2.dll");if(!module)return;
    auto get=reinterpret_cast<int(__cdecl*)()>(GetProcAddress(module,"SDL_GetRelativeMouseMode"));
    auto set=reinterpret_cast<int(__cdecl*)(int)>(GetProcAddress(module,"SDL_SetRelativeMouseMode"));
    if(!get||!set)return;
    if(release){if(!relativeOwned){previousRelative=get();relativeOwned=true;}if(get())set(0);}
    else if(relativeOwned){set(previousRelative);relativeOwned=false;}
}
bool respawnAttack=false;
uint32_t lastMask=0;
ULONGLONG lastMelee=0,lastTerrain=0;
int terrainCursor=0;
std::map<std::tuple<int,int,int>,CBaseHandle> blocks;
std::map<int,ULONGLONG> impacts;
float potionSpeed=1,potionJump=1,appliedSpeed=-1;
int potionFlags=0,appliedFlags=0;
ULONGLONG potionLease=0;
bool speedEntity=false,fireFilter=false;
bool nativeHidden=false;
bool guestMovement=false,movementOwned=false,movementGround=false;
bool movementBindings=false;
mcvalve::MotionQueue motion;bool motionStep=false;ULONGLONG motionTime=0;
int previousMoveType=MOVETYPE_WALK,movementFlags=0;
float movementEye=51.84f;
Vector originalEye(0,0,64);
bool gravityOwned=false;
float originalGravity=1;
CBaseHandle testTarget;
CBaseHandle supportEntity;
Vector supportLocal;
CBaseHandle testPlatform;
Vector platformOrigin;
ULONGLONG platformStarted=0;
edict_t* platformEdict(){auto* e=testPlatform.IsValid()?engine->PEntityOfEntIndex(testPlatform.GetEntryIndex()):nullptr;return e&&!e->IsFree()&&e->GetUnknown()&&e->GetUnknown()->GetRefEHandle()==testPlatform?e:nullptr;}
ULONGLONG testInputDeadline=0;
char originalDamageFilter[128]{};
Vector native(const float* p){float out[3];mcvalve::toHost(p,out);return Vector(out[0],out[1],out[2]);}
float eyeHeight(IPlayerInfo* p){
    // Read the live server offset: player thinking may reset a requested view
    // height before this public eye-position teleport helper runs.
    if(movementOwned&&engine&&tools){auto* e=engine->PEntityOfEntIndex(1);char text[128]{};Vector offset;
        if(e&&!e->IsFree()&&tools->GetKeyValue(e->GetUnknown()->GetBaseEntity(),"view_ofs",text,sizeof(text))&&std::sscanf(text,"%f %f %f",&offset.x,&offset.y,&offset.z)==3&&offset.z>0&&offset.z<=96)return offset.z;
    }
    return p->GetPlayerMaxs().z>40?64.f:28.f;
}
// The SDK tool accepts an EYE position, not an entity/feet origin.
bool snapFeet(IPlayerInfo* p,const Vector& foot,const QAngle& angles){bool ok=tools->SnapPlayerToPosition(foot+Vector(0,0,eyeHeight(p)),angles);if(ok&&engineClient){QAngle view=angles;engineClient->SetViewAngles(view);}return ok;}
float skill(const char* name,float fallback){const auto* variable=cvars?cvars->FindVar(name):nullptr;if(!variable)return fallback;float value=variable->GetFloat();return std::isfinite(value)?std::clamp(value,0.f,1000.f):fallback;}
struct Filter:ITraceFilter{IHandleEntity* ignore;IHandleEntity* support=nullptr;explicit Filter(IHandleEntity* p):ignore(p){}
    bool ShouldHitEntity(IHandleEntity* e,int)override{return e!=ignore&&e!=support;}
    TraceType_t GetTraceType()const override{return TRACE_EVERYTHING;}};
struct Damage:CTakeDamageInfo{
    Damage(IServerUnknown* player,float amount,int type,const Vector& at,const Vector& force){
        static_cast<CBaseHandle&>(m_hAttacker)=player->GetRefEHandle();
        static_cast<CBaseHandle&>(m_hInflictor)=player->GetRefEHandle();
        m_flDamage=m_flMaxDamage=amount;m_bitsDamageType=type;m_vecDamagePosition=at;m_vecDamageForce=force;
    }
};
void ack(const mcvalve::Packet& p,int status,int entity=0){mcvalve::Packet a;a.kind=mcvalve::Ack;a.sequence=p.sequence;a.i[0]=status;a.i[1]=entity;a.i[2]=p.i[0];link.send(a);}
void process(edict_t* edict,IPlayerInfo* info,const mcvalve::Packet& p){
    if(p.kind==mcvalve::FinishEnding){
        if(endingPhase!=2||p.i[0]<=0){ack(p,-1);return;}
        DWORD guestPid=DWORD(p.i[0]);
        EnumWindows([](HWND w,LPARAM data)->BOOL{DWORD pid=0;GetWindowThreadProcessId(w,&pid);if(pid==DWORD(data)&&IsWindowVisible(w)){ShowWindow(w,SW_RESTORE);SetForegroundWindow(w);return FALSE;}return TRUE;},LPARAM(guestPid));
        ack(p,1);engine->ServerCommand("quit\n");return;
    }
    auto* unknown=edict->GetUnknown();Filter filter(unknown);trace_t hit;Ray_t ray;
    Vector a=native(p.f),b=native(p.f+3);
    if(p.kind==mcvalve::Movement){
        if(!movementOwned||p.i[0]<0||p.i[0]>3){ack(p,-1);return;}
        if(!motionStep){movementFlags=p.i[0];motion.push({p.f[0],p.f[1],p.f[2]});return;}
        Vector foot=info->GetAbsOrigin();a=foot+Vector(-p.f[2]*32,-p.f[0]*32,p.f[1]*32);if((a-foot).Length()>512){ack(p,-1);return;}
        movementFlags=p.i[0];float height=(movementFlags&1)?19.2f:(movementFlags&2)?48.f:57.6f;
        auto trace=[&](mcvalve::MoveVec x,mcvalve::MoveVec y){
            Ray_t sweep;sweep.Init(Vector(x[0],x[1],x[2]),Vector(y[0],y[1],y[2]),Vector(-9.6f,-9.6f,0),Vector(9.6f,9.6f,height));
            trace_t h;traces->TraceRay(sweep,MASK_PLAYERSOLID,&filter,&h);
            return mcvalve::MoveHit{h.fraction,{h.plane.normal.x,h.plane.normal.y,h.plane.normal.z},h.startsolid||h.allsolid};
        };
        auto moved=mcvalve::move({foot.x,foot.y,foot.z},{a.x,a.y,a.z},trace,movementGround,(movementFlags&1)!=0);
        movementEye=(movementFlags&1)?13.6f:(movementFlags&2)?40.64f:51.84f;
        tools->SetKeyValue(unknown->GetBaseEntity(),"view_ofs",Vector(0,0,movementEye));
        QAngle look=info->GetAbsAngles();if(engineClient)engineClient->GetViewAngles(look);
        snapFeet(info,Vector(moved.feet[0],moved.feet[1],moved.feet[2]),look);edict->StateChanged();movementGround=moved.grounded;
        supportEntity.Term();
        if(movementGround){
            Ray_t down;down.Init(Vector(moved.feet[0],moved.feet[1],moved.feet[2]),Vector(moved.feet[0],moved.feet[1],moved.feet[2]-2),Vector(-9.6f,-9.6f,0),Vector(9.6f,9.6f,height));
            trace_t floor;traces->TraceRay(down,MASK_PLAYERSOLID,&filter,&floor);
            for(int n=1;n<globals->maxEntities&&floor.m_pEnt;n++){
                auto* e=engine->PEntityOfEntIndex(n);if(!e||e->IsFree()||!e->GetUnknown()||e->GetUnknown()->GetBaseEntity()!=floor.m_pEnt)continue;
                auto* c=e->GetCollideable();if(!c)break;
                supportEntity=e->GetUnknown()->GetRefEHandle();const auto& matrix=c->CollisionToWorldTransform();
                Vector offset=info->GetAbsOrigin()-Vector(matrix[0][3],matrix[1][3],matrix[2][3]);
                for(int axis=0;axis<3;axis++)supportLocal[axis]=offset.x*matrix[0][axis]+offset.y*matrix[1][axis]+offset.z*matrix[2][axis];
                break;
            }
        }
        mcvalve::Packet result;result.kind=mcvalve::MovementResult;mcvalve::toGuest(info->GetAbsOrigin().Base(),result.f);
        result.i[0]=movementGround;result.i[1]=moved.blocked;result.i[2]=movementFlags;link.send(result);return;
    }
    if(p.kind>=mcvalve::TestSpawn&&p.kind<=mcvalve::TestPlatform){
        if(!CommandLine()->FindParm("-minecraftify-tests")){ack(p,-2);return;}
        if(p.kind==mcvalve::TestPlatform){
            auto* platform=platformEdict();
            if(p.i[0]==0){if(platform)tools->RemoveEntity(platform->GetUnknown()->GetBaseEntity());testPlatform.Term();supportEntity.Term();ack(p,1);return;}
            if(p.i[0]==1){
                if(platform){ack(p,-1);return;}
                Vector destination=info->GetAbsOrigin()+Vector(6,0,32.03125f);
                Ray_t check;check.Init(destination,destination,Vector(-9.6f,-9.6f,0),Vector(9.6f,9.6f,57.6f));trace_t safe;traces->TraceRay(check,MASK_PLAYERSOLID,&filter,&safe);
                if(safe.startsolid||safe.allsolid){ack(p,-1);return;}
                auto* entity=tools->CreateEntityByName("prop_dynamic_override");if(!entity){ack(p,-1);return;}
                platformOrigin=info->GetAbsOrigin()+Vector(0,0,16);
                tools->SetKeyValue(entity,"model","models/minecraftify/block.mdl");tools->SetKeyValue(entity,"origin",platformOrigin);tools->SetKeyValue(entity,"solid","6");tools->SetKeyValue(entity,"rendermode","10");tools->DispatchSpawn(entity);
                for(int n=1;n<globals->maxEntities;n++){auto* e=engine->PEntityOfEntIndex(n);if(e&&!e->IsFree()&&e->GetUnknown()&&e->GetUnknown()->GetBaseEntity()==entity){testPlatform=e->GetUnknown()->GetRefEHandle();break;}}
                if(!testPlatform.IsValid()){tools->RemoveEntity(entity);ack(p,-1);return;}
                destination.z=platformOrigin.z+platformEdict()->GetCollideable()->OBBMaxs().z+.03125f;platformStarted=GetTickCount64()+500;snapFeet(info,destination,info->GetAbsAngles());motion.clear();supportEntity.Term();ack(p,1,testPlatform.GetEntryIndex());return;
            }
            if(p.i[0]==2&&platform){
                mcvalve::Packet out;out.kind=mcvalve::Ack;out.sequence=p.sequence;out.i[0]=1;out.i[1]=testPlatform.GetEntryIndex();out.i[2]=supportEntity==testPlatform;out.i[3]=movementGround;
                Ray_t floorProbe;floorProbe.Init(info->GetAbsOrigin()+Vector(0,0,4),info->GetAbsOrigin()-Vector(0,0,4),Vector(-9.6f,-9.6f,0),Vector(9.6f,9.6f,57.6f));trace_t probe;traces->TraceRay(floorProbe,MASK_PLAYERSOLID,&filter,&probe);
                out.i[4]=probe.startsolid;out.f[8]=probe.fraction;out.f[9]=probe.plane.normal.z;
                for(int n=1;n<globals->maxEntities;n++){auto* e=engine->PEntityOfEntIndex(n);if(e&&!e->IsFree()&&e->GetUnknown()&&e->GetUnknown()->GetBaseEntity()==probe.m_pEnt){out.i[5]=n;break;}}
                out.f[10]=platform->GetCollideable()->OBBMaxs().z;out.f[11]=platform->GetCollideable()->GetCollisionOrigin().z;
                out.i[6]=supportEntity.IsValid()?supportEntity.GetEntryIndex():-1;
                const auto& m=platform->GetCollideable()->CollisionToWorldTransform();Vector expected;
                for(int n=0;n<3;n++)expected[n]=m[n][0]*6+m[n][2]*(platform->GetCollideable()->OBBMaxs().z+.03125f)+m[n][3];
                out.f[0]=(info->GetAbsOrigin()-expected).Length()/32;mcvalve::toGuest(info->GetAbsOrigin().Base(),out.f+2);mcvalve::toGuest(expected.Base(),out.f+5);link.send(out);return;
            }
            ack(p,-1);return;
        }
        if(p.kind==mcvalve::TestInspect&&p.i[0]==98){mcvalve::Packet result;result.kind=mcvalve::Ack;result.sequence=p.sequence;result.i[0]=1;result.i[7]=movementOwned;result.i[6]=-1;char value[64]{};if(tools->GetKeyValue(unknown->GetBaseEntity(),"movetype",value,sizeof(value)))result.i[6]=atoi(value);mcvalve::toGuest(info->GetAbsOrigin().Base(),result.f+2);link.send(result);return;}
        if(p.kind==mcvalve::TestBlock||p.kind==mcvalve::TestRay){auto event=p;event.kind=p.kind==mcvalve::TestBlock?mcvalve::Block:mcvalve::Projectile;process(edict,info,event);return;}
        if(p.kind==mcvalve::TestInput){
            if(!engineClient){ack(p,-1);return;}
            engineClient->ClientCmd_Unrestricted("-forward;-back;-jump");testInputDeadline=0;
            if(p.i[0]>=1&&p.i[0]<=3){const char* commands[]={"","+forward","+back","+jump"};engineClient->ClientCmd_Unrestricted(commands[p.i[0]]);testInputDeadline=GetTickCount64()+DWORD(std::clamp(p.f[0],50.f,500.f));}
            ack(p,1);return;
        }
        if(p.kind==mcvalve::TestTeleport){
            Ray_t hull;hull.Init(a,a,info->GetPlayerMins(),info->GetPlayerMaxs());trace_t safe;traces->TraceRay(hull,MASK_PLAYERSOLID,&filter,&safe);
            if((a-info->GetAbsOrigin()).Length()>256||safe.startsolid||safe.allsolid){ack(p,-1);return;}
            bool moved=snapFeet(info,a,info->GetAbsAngles());tools->SetKeyValue(unknown->GetBaseEntity(),"velocity",Vector(0,0,0));ack(p,moved?1:-1);return;
        }
        auto* targetEdict=testTarget.IsValid()?engine->PEntityOfEntIndex(testTarget.GetEntryIndex()):nullptr;
        CBaseEntity* target=targetEdict&&!targetEdict->IsFree()&&targetEdict->GetUnknown()->GetRefEHandle()==testTarget?targetEdict->GetUnknown()->GetBaseEntity():nullptr;
        int inspectedIndex=testTarget.IsValid()?testTarget.GetEntryIndex():0;
        if(p.kind==mcvalve::TestInspect&&p.i[0]>=6&&p.i[0]<=9){
            const char* names[]={"npc_helicopter","npc_combinegunship","npc_strider","npc_antlionguard"};target=nullptr;targetEdict=nullptr;
            for(int n=1;n<globals->maxEntities;n++){auto* e=engine->PEntityOfEntIndex(n);if(e&&!e->IsFree()&&!strcmp(e->GetClassName(),names[p.i[0]-6])){targetEdict=e;target=e->GetUnknown()->GetBaseEntity();inspectedIndex=n;break;}}
        }
        if(p.kind==mcvalve::TestRemove){if(target)tools->RemoveEntity(target);testTarget.Term();ack(p,1);return;}
        if(p.kind==mcvalve::TestHealth){
            if(p.i[0]==1){Damage burn(unknown,std::clamp(p.f[0],1.f,25.f),DMG_BURN,info->GetAbsOrigin(),Vector(0,0,0));tools->ClearMultiDamage();tools->AddMultiDamage(burn,unknown->GetBaseEntity());tools->ApplyMultiDamage();}
            else {tools->SetKeyValue(unknown->GetBaseEntity(),"health",std::clamp(p.f[0],1.f,100.f));edict->StateChanged();}ack(p,1);return;
        }
        if(p.kind==mcvalve::TestAim){
            Vector eye=info->GetAbsOrigin()+Vector(0,0,eyeHeight(info)),delta=a-eye;QAngle angle(-atan2f(delta.z,sqrtf(delta.x*delta.x+delta.y*delta.y))*57.2957795f,atan2f(delta.y,delta.x)*57.2957795f,0);
            snapFeet(info,info->GetAbsOrigin(),angle);ack(p,1);return;
        }
        if(p.kind==mcvalve::TestSpawn){
            if(target)tools->RemoveEntity(target);testTarget.Term();
            if((a-info->GetAbsOrigin()).Length()>256){ack(p,-1);return;}
            Ray_t groundRay;groundRay.Init(a+Vector(0,0,96),a-Vector(0,0,128));trace_t ground;traces->TraceRay(groundRay,MASK_NPCSOLID,&filter,&ground);
            if(ground.fraction==1||ground.startsolid){ack(p,-1);return;}
            Vector foot=ground.endpos+Vector(0,0,1);Ray_t hull;hull.Init(foot,foot,Vector(-16,-16,0),Vector(16,16,72));trace_t safe;traces->TraceRay(hull,MASK_NPCSOLID,&filter,&safe);
            if(safe.startsolid||safe.allsolid){ack(p,-1);return;}
            target=tools->CreateEntityByName("npc_zombie");if(!target){ack(p,-1);return;}
            tools->SetKeyValue(target,"origin",foot);tools->SetKeyValue(target,"spawnflags","128");tools->SetKeyValue(target,"targetname","minecraftify_test_target");tools->DispatchSpawn(target);tools->SetMoveType(target,MOVETYPE_NONE);tools->SetKeyValue(target,"health","1000");tools->SetKeyValue(target,"max_health","1000");
            if(engineClient)engineClient->ClientCmd_Unrestricted("ent_fire minecraftify_test_target SetRelationship \"!player D_NU 99\"\n");
            for(int n=1;n<globals->maxEntities;n++){auto* e=engine->PEntityOfEntIndex(n);if(e&&!e->IsFree()&&e->GetUnknown()->GetBaseEntity()==target){testTarget=e->GetUnknown()->GetRefEHandle();targetEdict=e;break;}}
        }
        mcvalve::Packet result;result.kind=mcvalve::Ack;result.sequence=p.sequence;result.i[0]=target?1:0;
        if(result.i[0]){char hp[64]{},burrowed[64]{};tools->GetKeyValue(target,"health",hp,sizeof(hp));tools->GetKeyValue(target,"startburrowed",burrowed,sizeof(burrowed));result.f[0]=float(atof(hp));result.f[7]=float(atof(burrowed));result.i[1]=p.kind==mcvalve::TestSpawn?testTarget.GetEntryIndex():inspectedIndex;auto* collision=targetEdict->GetUnknown()->GetCollideable();result.i[3]=collision->GetSolidFlags();result.i[4]=collision->GetSolid();Vector center=collision->GetCollisionOrigin()+Vector(0,0,36);mcvalve::toGuest(center.Base(),result.f+2);}
        result.f[5]=skill("sk_plr_dmg_crowbar",25);result.f[6]=skill("sk_plr_dmg_pistol",5);link.send(result);return;
    }
    if(p.kind==mcvalve::Strike||p.kind==mcvalve::Projectile){
        if(p.kind==mcvalve::Projectile&&impacts.count(p.i[0])){ack(p,0);return;}
        if(p.kind==mcvalve::Strike){
            if(GetTickCount64()-lastMelee<450){ack(p,0);return;}lastMelee=GetTickCount64();
            a=info->GetAbsOrigin()+Vector(0,0,eyeHeight(info));auto command=info->GetLastUserCommand();
            float yaw=command.viewangles.y*0.0174532925f,pitch=command.viewangles.x*0.0174532925f;
            b=a+Vector(cosf(pitch)*cosf(yaw),cosf(pitch)*sinf(yaw),-sinf(pitch))*80;
        }
        if((b-a).Length()>4096){ack(p,-1);return;}
        ray.Init(a,b);traces->TraceRay(ray,MASK_SHOT,&filter,&hit);
        if(hit.fraction<1){
            if(p.kind==mcvalve::Projectile)impacts[p.i[0]]=GetTickCount64();
            if(p.i[1]==3){
                Vector destination=hit.endpos+hit.plane.normal*20;
                Ray_t hull;hull.Init(destination,destination,info->GetPlayerMins(),info->GetPlayerMaxs());trace_t safe;
                traces->TraceRay(hull,MASK_PLAYERSOLID,&filter,&safe);
                if(!safe.startsolid&&!safe.allsolid&&snapFeet(info,destination,info->GetAbsAngles())){
                    tools->SetKeyValue(unknown->GetBaseEntity(),"velocity",Vector(0,0,0));
                    Damage pearlDamage(unknown,25,DMG_FALL,destination,Vector(0,0,0));tools->ClearMultiDamage();tools->AddMultiDamage(pearlDamage,unknown->GetBaseEntity());tools->ApplyMultiDamage();
                }
            }else if(p.i[1]==5){Damage damage(unknown,100,DMG_BLAST,hit.endpos,Vector(0,0,1000));tools->RadiusDamage(damage,hit.endpos,192,0,nullptr);}
            else if(hit.m_pEnt&&(p.kind==mcvalve::Strike||p.i[1]==1)){
                Damage damage(unknown,p.kind==mcvalve::Strike?skill("sk_plr_dmg_crowbar",25):30.f*std::clamp(p.f[6],0.1f,4.f),p.kind==mcvalve::Strike?DMG_CLUB:DMG_BULLET,hit.endpos,(b-a)*30);
                tools->ClearMultiDamage();tools->AddMultiDamage(damage,hit.m_pEnt);tools->ApplyMultiDamage();
            }
            int hitIndex=0;
            for(int n=0;n<globals->maxEntities;n++){auto* e=engine->PEntityOfEntIndex(n);if(e&&!e->IsFree()&&e->GetUnknown()&&e->GetUnknown()->GetBaseEntity()==hit.m_pEnt){hitIndex=n;break;}}
            {mcvalve::Packet out;out.kind=mcvalve::Ack;out.sequence=p.sequence;out.i[0]=1;out.i[1]=hitIndex;mcvalve::toGuest(hit.endpos.Base(),out.f);link.send(out);}
        }else ack(p,0);
    }else if(p.kind==mcvalve::Blast){
        if((a-info->GetAbsOrigin()).Length()>8192){ack(p,-1);return;}
        // Whole-block guest floor sampling can leave a blast just inside BSP.
        // Raise it to the nearest open surface before the engine's LOS checks.
        for(int n=0;n<16&&(traces->GetPointContents(a)&CONTENTS_SOLID);n++)a.z+=4;
        if(traces->GetPointContents(a)&CONTENTS_SOLID){ack(p,-1);return;}
        Damage damage(unknown,150.f*std::clamp(p.f[6],0.1f,2.f),DMG_BLAST,a,Vector(0,0,1000));tools->RadiusDamage(damage,a,256,0,nullptr);ack(p,1);
    }else if(p.kind==mcvalve::Block){
        auto key=std::make_tuple(p.i[0],p.i[1],p.i[2]);auto found=blocks.find(key);
        if(found!=blocks.end()){
            auto* e=engine->PEntityOfEntIndex(found->second.GetEntryIndex());
            if(e&&!e->IsFree()&&e->GetUnknown()->GetRefEHandle()==found->second){if(!p.i[3])tools->RemoveEntity(e->GetUnknown()->GetBaseEntity());}
            if(!p.i[3])blocks.erase(found);ack(p,1);return;
        }
        if(!p.i[3]){ack(p,1);return;}
        float guest[3]={p.i[0]+.5f,p.i[1]+.5f,p.i[2]+.5f};Vector center=native(guest);
        if(blocks.size()>=512||(center-info->GetAbsOrigin()).Length()>256){ack(p,-1);return;}
        auto* block=tools->CreateEntityByName("prop_dynamic_override");if(!block){ack(p,-1);return;}
        tools->SetKeyValue(block,"model","models/minecraftify/block.mdl");tools->SetKeyValue(block,"origin",center);
        tools->SetKeyValue(block,"solid","6");tools->SetKeyValue(block,"rendermode","10");tools->SetKeyValue(block,"renderamt","0");
        tools->SetKeyValue(block,"disableshadows","1");tools->DispatchSpawn(block);
        for(int n=1;n<globals->maxEntities;n++){
            auto* e=engine->PEntityOfEntIndex(n);if(e&&!e->IsFree()&&e->GetUnknown()->GetBaseEntity()==block){blocks.emplace(key,e->GetUnknown()->GetRefEHandle());ack(p,1,n);return;}
        }
        tools->RemoveEntity(block);ack(p,-1);
    }else if(p.kind==mcvalve::Health){
        float hp=std::clamp(float(info->GetHealth())+p.f[0],0.f,float(info->GetMaxHealth()));
        bool ok=tools->SetKeyValue(unknown->GetBaseEntity(),"health",hp);if(ok)edict->StateChanged();ack(p,ok?1:-1);
    }else if(p.kind==mcvalve::Effect){
        potionSpeed=std::clamp(p.f[0],1.f,2.f);potionJump=std::clamp(p.f[1],1.f,2.f);potionFlags=p.i[0]&7;potionLease=GetTickCount64();ack(p,(potionFlags&4)?-2:1);
    }else if(p.kind==mcvalve::ChangeMap){
        if(p.i[0]<0||p.i[0]>=sizeof(mcvalve::halflife2Maps)/sizeof(*mcvalve::halflife2Maps)){ack(p,-1);return;}
        char command[128];std::snprintf(command,sizeof(command),"map %s\nexec minecraftify.cfg\n",mcvalve::halflife2Maps[p.i[0]]);ack(p,1);engine->ServerCommand(command);
    }else if(p.kind==mcvalve::Travel){
        const char* names[]={"npc_helicopter","npc_combinegunship","npc_strider","npc_antlionguard"};
        if(p.i[0]<6||p.i[0]>9){ack(p,-1);return;}
        edict_t* boss=nullptr;
        for(int n=1;n<globals->maxEntities;n++){auto* e=engine->PEntityOfEntIndex(n);if(e&&!e->IsFree()&&!strcmp(e->GetClassName(),names[p.i[0]-6])){boss=e;break;}}
        bool moved=false,found=false,hasFallback=false;Vector chosen,fallback;
        if(boss){
            Vector origin=boss->GetUnknown()->GetCollideable()->GetCollisionOrigin();
            for(int n=0;n<120&&!found;n++){
                const float heights[]={64,128,256,512,1024};int spot=n%24;
                float angle=(spot%8)*.785398163f,distance=384.f+(spot/8)*128.f;
                Vector above=origin+Vector(cosf(angle)*distance,sinf(angle)*distance,heights[n/24]);
                Ray_t groundRay;groundRay.Init(above,above-Vector(0,0,8192));trace_t ground;
                traces->TraceRay(groundRay,MASK_PLAYERSOLID,&filter,&ground);if(ground.fraction==1||ground.startsolid)continue;
                Vector destination=ground.endpos+Vector(0,0,1);Ray_t hull;hull.Init(destination,destination,info->GetPlayerMins(),info->GetPlayerMaxs());trace_t safe;
                traces->TraceRay(hull,MASK_PLAYERSOLID,&filter,&safe);if(safe.startsolid||safe.allsolid)continue;
                if(!hasFallback){fallback=destination;hasFallback=true;}
                Ray_t sightRay;sightRay.Init(destination+Vector(0,0,eyeHeight(info)),origin+Vector(0,0,36));trace_t sight;traces->TraceRay(sightRay,MASK_SHOT,&filter,&sight);
                if(sight.fraction<.99f&&sight.m_pEnt!=boss->GetUnknown()->GetBaseEntity())continue;
                chosen=destination;found=true;
            }
            if(!found&&hasFallback){chosen=fallback;found=true;}
            if(found){Vector delta=origin+Vector(0,0,36)-(chosen+Vector(0,0,eyeHeight(info)));QAngle facing(-atan2f(delta.z,sqrtf(delta.x*delta.x+delta.y*delta.y))*57.2957795f,atan2f(delta.y,delta.x)*57.2957795f,0);moved=snapFeet(info,chosen,facing);}
        }
        if(moved&&boss){
            char target[128]{};tools->GetKeyValue(boss->GetUnknown()->GetBaseEntity(),"targetname",target,sizeof(target));
            bool safe=target[0]!=0;for(const char* c=target;*c;c++)if(!std::isalnum(static_cast<unsigned char>(*c))&&*c!='_'&&*c!='-'&&*c!='.')safe=false;
            const char* name=safe?target:names[p.i[0]-6];char command[384];
            std::snprintf(command,sizeof(command),"ent_fire %s Wake\n",name);if(engineClient)engineClient->ClientCmd_Unrestricted(command);
            if(p.i[0]==9){
                std::snprintf(command,sizeof(command),"ent_fire %s Unburrow\n",name);if(engineClient)engineClient->ClientCmd_Unrestricted(command);
                char value[64]{};auto* entity=boss->GetUnknown()->GetBaseEntity();if(tools->GetKeyValue(entity,"effects",value,sizeof(value))){tools->SetKeyValue(entity,"effects",float(atoi(value)&~EF_NODRAW));boss->ClearTransmitState();boss->StateChanged();}
            }
        }
        ack(p,moved?1:-3);
    }else ack(p,-2);
}
class Plugin:public IServerPluginCallbacks {
public:
 bool Load(CreateInterfaceFn factory,CreateInterfaceFn game)override{
    engine=static_cast<IVEngineServer*>(factory(INTERFACEVERSION_VENGINESERVER,nullptr));
    cvars=static_cast<ICvar*>(factory(CVAR_INTERFACE_VERSION,nullptr));
    players=static_cast<IPlayerInfoManager*>(game(INTERFACEVERSION_PLAYERINFOMANAGER,nullptr));
    tools=static_cast<IServerTools*>(game(VSERVERTOOLS_INTERFACE_VERSION,nullptr));
    traces=static_cast<IEngineTrace*>(factory(INTERFACEVERSION_ENGINETRACE_SERVER,nullptr));
    if(!engine||!players||!tools||!traces)return false;
    globals=players->GetGlobalVars();
    return link.open(25612,25622);
 }
 void Unload()override{restoreEndingRender();relativeMouse(false);if(mouseReleased&&client)client->IN_ActivateMouse();} void Pause()override{paused=true;}void UnPause()override{paused=false;}
 const char* GetPluginDescription()override{return "Minecraftify Anything / Half-Life 2 development adapter";}
 void LevelInit(const char* name)override{endingPhase=0;endingNotice=0;strncpy(mapName,name,63);link.scene=uint32_t(GetTickCount64());link.lastSequence=0;blocks.clear();impacts.clear();testTarget.Term();testPlatform.Term();supportEntity.Term();motion.clear();nativeHidden=false;speedEntity=fireFilter=false;appliedSpeed=-1;potionFlags=appliedFlags=0;potionSpeed=potionJump=1;originalDamageFilter[0]=0;guestMovement=movementOwned=movementBindings=false;movementFlags=0;}
 void ServerActivate(edict_t*,int,int)override{}
 void GameFrame(bool simulating)override{
    if(paused||!globals||globals->maxClients!=1)return;
    auto* edict=engine->PEntityOfEntIndex(1);if(!edict||edict->IsFree())return;
    auto* player=players->GetPlayerInfo(edict);if(!player||!player->IsConnected())return;
    endingState();
    mcvalve::Packet p;
    for(int n=0;n<128&&link.receive(p);n++){
        if(p.kind==mcvalve::Heartbeat){if(p.scene==link.scene){if(!link.active()){link.lastSequence=0;impacts.clear();}link.lease=GetTickCount64();inventory=p.i[0]!=0;chat=p.i[2]==1;guestMovement=p.i[1]==1;}continue;}
        if(!simulating||!link.active()||p.scene!=link.scene||p.sequence<=link.lastSequence)continue;
        link.lastSequence=p.sequence;
        if(!player->IsDead()||p.kind==mcvalve::FinishEnding||p.kind==mcvalve::TestInspect||p.kind==mcvalve::TestRemove||p.kind==mcvalve::Projectile||p.kind==mcvalve::Blast||p.kind==mcvalve::Block)process(edict,player,p);
        else ack(p,0);
    }
    bool endingScene=!strcmp(mapName,"mc_gman_ending")||(!strcmp(mapName,"d3_breen_01")&&(endingPhase||endingCinematic));
    bool wantMovement=link.active()&&guestMovement&&!player->IsDead()&&!endingScene;
    if(simulating&&link.active())if(auto* platform=platformEdict()){
        float t=GetTickCount64()>platformStarted?float(GetTickCount64()-platformStarted)/1000.f:0;
        tools->SetKeyValue(platform->GetUnknown()->GetBaseEntity(),"origin",platformOrigin+Vector(16*sinf(t),12*sinf(t*.7f),8*sinf(t*.9f)));
        char angles[64];std::snprintf(angles,sizeof(angles),"0 %.3f 0",25*sinf(t*.8f));tools->SetKeyValue(platform->GetUnknown()->GetBaseEntity(),"angles",angles);platform->StateChanged();
    }
    auto* playerEntity=edict->GetUnknown()->GetBaseEntity();
    if(wantMovement&&!movementOwned){
        char value[128]{};previousMoveType=MOVETYPE_WALK;if(tools->GetKeyValue(playerEntity,"movetype",value,sizeof(value)))previousMoveType=atoi(value);
        originalEye=Vector(0,0,64);if(tools->GetKeyValue(playerEntity,"view_ofs",value,sizeof(value)))std::sscanf(value,"%f %f %f",&originalEye.x,&originalEye.y,&originalEye.z);
        movementOwned=true;movementGround=true;motion.clear();motionTime=GetTickCount64();tools->SetMoveType(playerEntity,MOVETYPE_FLY);
    }
    if(!wantMovement&&movementOwned){tools->SetMoveType(playerEntity,previousMoveType);tools->SetKeyValue(playerEntity,"view_ofs",originalEye);edict->StateChanged();movementOwned=false;movementFlags=0;motion.clear();supportEntity.Term();}
    if(movementOwned){
        tools->SetMoveType(playerEntity,MOVETYPE_FLY);tools->SetKeyValue(playerEntity,"velocity",Vector(0,0,0));
        if(supportEntity.IsValid()){
            auto* support=engine->PEntityOfEntIndex(supportEntity.GetEntryIndex());
            if(support&&!support->IsFree()&&support->GetUnknown()&&support->GetUnknown()->GetRefEHandle()==supportEntity&&support->GetCollideable()){
                const auto& m=support->GetCollideable()->CollisionToWorldTransform();Vector target;
                for(int n=0;n<3;n++)target[n]=m[n][0]*supportLocal.x+m[n][1]*supportLocal.y+m[n][2]*supportLocal.z+m[n][3];
                Vector from=player->GetAbsOrigin();
                if((target-from).Length()<=128){
                    Filter carry(edict->GetUnknown());carry.support=support->GetUnknown();
                    Ray_t sweep;sweep.Init(from,target,Vector(-9.6f,-9.6f,0),Vector(9.6f,9.6f,(movementFlags&1)?19.2f:(movementFlags&2)?48.f:57.6f));
                    trace_t safe;traces->TraceRay(sweep,MASK_PLAYERSOLID,&carry,&safe);
                    if(!safe.startsolid&&!safe.allsolid){QAngle look=player->GetAbsAngles();if(engineClient)engineClient->GetViewAngles(look);snapFeet(player,safe.endpos,look);edict->StateChanged();}
                }
            }
        }
        auto now=GetTickCount64();float dt=float(now-motionTime)/1000.f;motionTime=now;
        auto delta=motion.take(dt);mcvalve::Packet step;step.kind=mcvalve::Movement;step.i[0]=movementFlags;
        for(int n=0;n<3;n++)step.f[n]=delta[n];motionStep=true;process(edict,player,step);motionStep=false;
    }
    if(!client){auto module=GetModuleHandleA("client.dll");if(module){auto f=reinterpret_cast<CreateInterfaceFn>(GetProcAddress(module,"CreateInterface"));if(f){client=static_cast<IBaseClientDLL*>(f(CLIENT_DLL_INTERFACE_VERSION,nullptr));clientTools=static_cast<IClientTools*>(f(VCLIENTTOOLS_INTERFACE_VERSION,nullptr));}}}
    if(!engineClient){auto module=GetModuleHandleA("engine.dll");if(module){auto f=reinterpret_cast<CreateInterfaceFn>(GetProcAddress(module,"CreateInterface"));if(f)engineClient=static_cast<IVEngineClient*>(f(VENGINE_CLIENT_INTERFACE_VERSION,nullptr));}}
    DWORD processId=0;HWND window=GetForegroundWindow();GetWindowThreadProcessId(window,&processId);bool focused=processId==GetCurrentProcessId();
    if(engineClient&&movementBindings!=movementOwned){
        engineClient->ClientCmd_Unrestricted(movementOwned?"-forward;-back;-moveleft;-moveright;-jump;-duck;bind w \"\";bind s \"\";bind a \"\";bind d \"\";bind SPACE \"\";bind SHIFT \"\"":"bind w +forward;bind s +back;bind a +moveleft;bind d +moveright;bind SPACE +jump;bind SHIFT +duck");movementBindings=movementOwned;
    }
    uint32_t mask=0;const int keys[]={VK_LBUTTON,VK_RBUTTON,'I',VK_F5,VK_ESCAPE,'Q','F',VK_MBUTTON,VK_SHIFT,VK_CONTROL};
    for(int n=0;n<10;n++)if(focused&&(GetAsyncKeyState(keys[n])&0x8001))mask|=1u<<n;
    for(int n=0;n<9;n++)if(focused&&(GetAsyncKeyState('1'+n)&0x8001))mask|=1u<<(10+n);
    if(focused&&(GetAsyncKeyState(VK_F8)&0x8001))mask|=1u<<20;
    if(focused&&(GetAsyncKeyState(VK_F6)&0x8001))mask|=1u<<21;
    const int menuKeys[]={VK_F9,VK_F10,VK_PRIOR,VK_NEXT};for(int n=0;n<4;n++)if(focused&&(GetAsyncKeyState(menuKeys[n])&0x8001))mask|=1u<<(27+n);
    const int movementKeys[]={'W','S','A','D',VK_SPACE};for(int n=0;n<5;n++)if(focused&&(GetAsyncKeyState(movementKeys[n])&0x8001))mask|=1u<<(22+n);
    if(focused&&(GetAsyncKeyState('T')&0x8001))mask|=1u<<31;
    textInput.poll(link,focused&&link.active()&&chat);
    mcvalve::Packet input;input.kind=mcvalve::Input;input.i[0]=mask;
    RECT rect{};POINT cursor{};GetClientRect(window,&rect);GetCursorPos(&cursor);ScreenToClient(window,&cursor);
    input.i[7]=(inventory?1:0)|(relativeOwned?4:0)|(surfaceReleased?8:0)|(mouseSurface?16:0);
    if(mouseSurface&&mouseSurface->IsCursorVisible())input.i[7]|=32;
    if(auto sdl=GetModuleHandleA("SDL2.dll")){auto get=reinterpret_cast<int(__cdecl*)()>(GetProcAddress(sdl,"SDL_GetRelativeMouseMode"));if(get&&get())input.i[7]|=64;}
    input.f[0]=rect.right>0?float(cursor.x)/rect.right:0;input.f[1]=rect.bottom>0?float(cursor.y)/rect.bottom:0;link.send(input);
    if(client&&focused&&link.active()){
        bool release=inventory;
        relativeMouse(release);
        if(release)client->IN_DeactivateMouse();else if(mouseReleased)client->IN_ActivateMouse();mouseReleased=release;
        if(engineClient&&!chat&&(mask&8)&&!(lastMask&8))engineClient->ClientCmd_Unrestricted(clientTools&&clientTools->IsRenderingThirdPerson()?"firstperson":"thirdperson");
    }else if(client){
        // Never recapture the desktop mouse while another window has focus.
        if(!focused&&!mouseReleased){relativeMouse(true);client->IN_DeactivateMouse();mouseReleased=true;}
        else if(mouseReleased){relativeMouse(false);client->IN_ActivateMouse();mouseReleased=false;}
    }
    lastMask=mask;
    if(testInputDeadline&&(GetTickCount64()>testInputDeadline||!link.active())){if(engineClient)engineClient->ClientCmd_Unrestricted("-forward;-back;-jump");testInputDeadline=0;}
    if(engineClient&&focused){
        bool attack=player->IsDead()&&(mask&1);
        if(attack!=respawnAttack){engineClient->ClientCmd_Unrestricted(attack?"+attack":"-attack");respawnAttack=attack;}
    }
    if(!link.active()||GetTickCount64()-potionLease>1000){potionSpeed=potionJump=1;potionFlags=0;}
    if(link.active()&&!speedEntity){
        auto* modifier=tools->CreateEntityByName("player_speedmod");
        if(modifier){tools->SetKeyValue(modifier,"targetname","minecraftify_speed");tools->SetKeyValue(modifier,"spawnflags","0");tools->DispatchSpawn(modifier);speedEntity=true;}
    }
    float wantedSpeed=link.active()&&inventory?0:potionSpeed;
    if(speedEntity&&engineClient&&wantedSpeed!=appliedSpeed){char command[128];std::snprintf(command,sizeof(command),"ent_fire minecraftify_speed ModifySpeed %.3f\n",wantedSpeed);engineClient->ClientCmd_Unrestricted(command);appliedSpeed=wantedSpeed;}
    if(link.active()&&potionJump!=1){
        if(!gravityOwned){char previous[64]{};tools->GetKeyValue(edict->GetUnknown()->GetBaseEntity(),"gravity",previous,sizeof(previous));originalGravity=float(atof(previous));if(originalGravity<=0)originalGravity=1;gravityOwned=true;}
        tools->SetKeyValue(edict->GetUnknown()->GetBaseEntity(),"gravity",originalGravity/(potionJump*potionJump));
    }else if(gravityOwned){tools->SetKeyValue(edict->GetUnknown()->GetBaseEntity(),"gravity",originalGravity);gravityOwned=false;}
    if(engineClient&&((potionFlags^appliedFlags)&2))engineClient->ClientCmd_Unrestricted("notarget");
    if(((potionFlags^appliedFlags)&1)){
        if(potionFlags&1){
            tools->GetKeyValue(edict->GetUnknown()->GetBaseEntity(),"damagefilter",originalDamageFilter,sizeof(originalDamageFilter));
            // Preserve a campaign's existing special damage rules rather than replacing them.
            if(!originalDamageFilter[0]){
                if(!fireFilter){auto* filter=tools->CreateEntityByName("filter_damage_type");if(filter){tools->SetKeyValue(filter,"targetname","minecraftify_no_fire");tools->SetKeyValue(filter,"damagetype","8");tools->SetKeyValue(filter,"Negated","1");tools->DispatchSpawn(filter);fireFilter=true;}}
                if(fireFilter&&engineClient)engineClient->ClientCmd_Unrestricted("ent_fire !player SetDamageFilter minecraftify_no_fire\n");
            }
        }else if(!originalDamageFilter[0]&&engineClient)engineClient->ClientCmd_Unrestricted("ent_fire !player SetDamageFilter \"\"\n");
    }
    appliedFlags=potionFlags;
    if(link.active()&&!nativeHidden){tools->SetKeyValue(edict->GetUnknown()->GetBaseEntity(),"rendermode","10");edict->StateChanged();nativeHidden=true;}
    else if(!link.active()&&nativeHidden){tools->SetKeyValue(edict->GetUnknown()->GetBaseEntity(),"rendermode","0");edict->StateChanged();nativeHidden=false;}
    CViewSetup view;if(client&&client->GetPlayerView(view)){
        mcvalve::Packet cam;cam.kind=mcvalve::Camera;cam.sequence=++sequence;
        mcvalve::toGuest(view.origin.Base(),cam.f);mcvalve::toGuest(player->GetAbsOrigin().Base(),cam.f+3);
        Vector eye=player->GetAbsOrigin()+Vector(0,0,player->GetPlayerMaxs().z>40?64:28);QAngle eyeAngles;float eyeFov=0;
        if(clientTools)clientTools->GetLocalPlayerEyePosition(eye,eyeAngles,eyeFov);mcvalve::toGuest(eye.Base(),cam.f+10);
        cam.f[6]=180.f-view.angles.y;cam.f[7]=view.angles.x;cam.f[8]=-view.angles.z;
        cam.f[9]=mcvalve::verticalFov(view.fov,view.height>0?float(view.width)/view.height:16.f/9.f);
        Ray_t floorRay;Vector foot=player->GetAbsOrigin();floorRay.Init(foot+Vector(0,0,.5f),foot-Vector(0,0,2),player->GetPlayerMins(),player->GetPlayerMaxs());Filter floorFilter(edict->GetUnknown());trace_t floorHit;traces->TraceRay(floorRay,MASK_PLAYERSOLID,&floorFilter,&floorHit);
        cam.i[0]=focused&&simulating&&!player->IsDead()&&!endingPhase&&!endingCinematic;cam.i[1]=!(clientTools&&clientTools->IsRenderingThirdPerson());cam.i[2]=view.width;cam.i[3]=view.height;cam.i[4]=movementOwned?movementGround:(floorHit.fraction<1&&!floorHit.startsolid);cam.i[5]=movementOwned?(movementFlags&2)!=0:player->GetPlayerMaxs().z<40;cam.i[6]=movementOwned;
        compositePose.publish({cam.f[0],cam.f[1],cam.f[2],cam.f[6],cam.f[7],cam.f[8],cam.f[9],view.height>0?float(view.width)/view.height:16.f/9});link.send(cam);
    }
    if(link.active()&&GetTickCount64()-lastTerrain>50){
        lastTerrain=GetTickCount64();float guest[3];mcvalve::toGuest(player->GetAbsOrigin().Base(),guest);
        for(int count=0;count<48;count++){
            int n=terrainCursor++%729;int x=int(floorf(guest[0]))+n%9-4,y=int(floorf(guest[1]))+(n/9)%9-4,z=int(floorf(guest[2]))+n/81-4;
            float center[3]={x+.5f,y+.5f,z+.5f};Vector pos=native(center);IHandleEntity* hitEntity=nullptr;
            int contents=traces->GetPointContents(pos,&hitEntity);
            auto* world=engine->PEntityOfEntIndex(0);
            mcvalve::Packet cell;cell.kind=mcvalve::Terrain;cell.i[0]=x;cell.i[1]=y;cell.i[2]=z;cell.i[3]=(contents&CONTENTS_SOLID)&&(!hitEntity||(world&&world->GetUnknown()==hitEntity));link.send(cell);
        }
    }
    for(auto at=impacts.begin();at!=impacts.end();)if(GetTickCount64()-at->second>10000)at=impacts.erase(at);else ++at;
    mcvalve::Packet vitals;vitals.kind=mcvalve::Vitals;vitals.f[0]=float(player->GetHealth());vitals.f[1]=float(player->GetMaxHealth());vitals.i[1]=int(blocks.size());vitals.i[2]=link.active();vitals.i[3]=link.receivedKind;vitals.i[4]=link.receivedPort;vitals.i[5]=link.receivedSize;vitals.i[6]=link.receivedValid;vitals.i[7]=int(link.lastSequence);link.send(vitals);
    mcvalve::Packet map;map.kind=mcvalve::Scene;memcpy(map.f,mapName,64);link.send(map);
 }
 void LevelShutdown()override{restoreEndingRender();endingPortalOpen=endingActorGone=endingCinematic=false;link.lease=0;gravityOwned=false;}
 void ClientActive(edict_t*)override{}void ClientDisconnect(edict_t*)override{}
 void ClientPutInServer(edict_t*,const char*)override{}void SetCommandClient(int)override{}void ClientSettingsChanged(edict_t*)override{}
 PLUGIN_RESULT ClientConnect(bool*,edict_t*,const char*,const char*,char*,int)override{return PLUGIN_CONTINUE;}
 PLUGIN_RESULT ClientCommand(edict_t*,const CCommand&)override{return PLUGIN_CONTINUE;}
 PLUGIN_RESULT NetworkIDValidated(const char*,const char*)override{return PLUGIN_CONTINUE;}
 void OnQueryCvarValueFinished(QueryCvarCookie_t,edict_t*,EQueryCvarValueStatus,const char*,const char*)override{}
 void OnEdictAllocated(edict_t*)override{}void OnEdictFreed(const edict_t*)override{}
} plugin;
}
IChangeInfoAccessor* CBaseEdict::GetChangeAccessor(){return engine->GetChangeAccessor(static_cast<edict_t*>(this));}
extern "C" __declspec(dllexport) void* CreateInterface(const char* name,int* status){
    if(!strcmp(name,INTERFACEVERSION_ISERVERPLUGINCALLBACKS)){if(status)*status=0;return &plugin;}
    if(status)*status=1;return nullptr;
}

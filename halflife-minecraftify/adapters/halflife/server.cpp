// Fresh adapter against Valve's official HL1 SDK; no HalfCraft code.
#include "../common/wire.hpp"
#include "../common/movement.hpp"
#include "../common/moments.hpp"
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
#include <map>
#include <tuple>
#include <cstdio>
#undef clamp

namespace {
mcvalve::Link link;
std::map<std::tuple<int,int,int>,EHANDLE> blocks;
float lastTime=-1,lastMelee=-1;
float speed=1,jump=1;
int effectFlags=0;
ULONGLONG effectTime=0;
bool ownedNoTarget=false;
bool guestMovement=false,movementOwned=false,movementGround=false;
mcvalve::MotionQueue motion;bool motionStep=false;ULONGLONG motionTime=0;
int previousMoveType=MOVETYPE_WALK,movementFlags=0;
void movementResult(CBasePlayer* player,int blocked){
    mcvalve::Packet out;out.kind=mcvalve::MovementResult;
    Vector feet=player->pev->origin-Vector(0,0,36);mcvalve::toGuest(feet,out.f);
    out.i[0]=movementGround;out.i[1]=blocked;out.i[2]=movementFlags;link.send(out);
}
char currentMap[64]{};
int terrainCursor=0;
ULONGLONG lastTerrain=0;
std::map<int,ULONGLONG> impacts;
EHANDLE testTarget;
ULONGLONG testInputDeadline=0;
uint32_t sceneCounter=0;
Vector native(const float* p){float v[3];mcvalve::toHost(p,v);return Vector(v[0],v[1],v[2]);}
void ack(const mcvalve::Packet& in,int status,int entity=0){mcvalve::Packet p;p.kind=mcvalve::Ack;p.sequence=in.sequence;p.i[0]=status;p.i[1]=entity;p.i[2]=in.i[0];link.send(p);}
void orient(CBasePlayer* player,Vector angles){
    player->pev->v_angle=angles;player->pev->angles=angles;player->pev->fixangle=1;
    mcvalve::Packet view;view.kind=mcvalve::Ui;view.i[0]=1;view.f[0]=angles.x;view.f[1]=angles.y;view.f[2]=angles.z;link.send(view);
}
void damage(CBasePlayer* player,TraceResult& tr,float amount,int type){
    auto* target=CBaseEntity::Instance(tr.pHit);if(!target||target==player||!target->pev->takedamage)return;
    ClearMultiDamage();target->TraceAttack(player->pev,amount,(tr.vecEndPos-player->EyePosition()).Normalize(),&tr,type);ApplyMultiDamage(player->pev,player->pev);
}
void process(CBasePlayer* player,const mcvalve::Packet& p){
    Vector start=native(p.f),end=native(p.f+3);TraceResult tr{};
    switch(p.kind){
    case mcvalve::Movement: {
        if(!movementOwned||p.i[0]<0||p.i[0]>3){ack(p,-1);break;}
        if(!motionStep){movementFlags=p.i[0];motion.push({p.f[0],p.f[1],p.f[2]});break;}
        Vector feet=player->pev->origin-Vector(0,0,36),target=feet+Vector(-p.f[2]*32,-p.f[0]*32,p.f[1]*32);
        if((target-feet).Length()>512){movementResult(player,7);break;}
        movementFlags=p.i[0];bool low=(movementFlags&3)!=0;float center=low?18.f:36.f;
        auto trace=[&](mcvalve::MoveVec a,mcvalve::MoveVec b){
            TraceResult h;Vector x(a[0],a[1],a[2]+center),y(b[0],b[1],b[2]+center);
            UTIL_TraceHull(x,y,dont_ignore_monsters,low?head_hull:human_hull,player->edict(),&h);
            return mcvalve::MoveHit{h.flFraction,{h.vecPlaneNormal.x,h.vecPlaneNormal.y,h.vecPlaneNormal.z},bool(h.fStartSolid||h.fAllSolid)};
        };
        auto moved=mcvalve::move({feet.x,feet.y,feet.z},{target.x,target.y,target.z},trace,movementGround,(movementFlags&1)!=0);
        UTIL_SetOrigin(player->pev,Vector(moved.feet[0],moved.feet[1],moved.feet[2]+36));player->pev->velocity=g_vecZero;
        movementGround=moved.grounded;
        if(movementGround){
            player->pev->flags|=FL_ONGROUND;TraceResult floor;Vector supportCenter=Vector(moved.feet[0],moved.feet[1],moved.feet[2]+center);
            UTIL_TraceHull(supportCenter,supportCenter-Vector(0,0,2),dont_ignore_monsters,low?head_hull:human_hull,player->edict(),&floor);
            player->pev->groundentity=floor.pHit;
        }else{player->pev->flags&=~FL_ONGROUND;player->pev->groundentity=nullptr;}
        player->pev->view_ofs=Vector(0,0,((movementFlags&1)?13.6f:(movementFlags&2)?40.64f:51.84f)-36);
        movementResult(player,moved.blocked);break;
    }
    case mcvalve::TestSpawn:case mcvalve::TestInspect:case mcvalve::TestRemove:case mcvalve::TestAim:case mcvalve::TestHealth:case mcvalve::TestInput:case mcvalve::TestTeleport:case mcvalve::TestBlock:case mcvalve::TestRay: {
        if(!g_engfuncs.pfnCheckParm("-minecraftify-tests",nullptr)){ack(p,-2);break;}
        if(p.kind==mcvalve::TestBlock||p.kind==mcvalve::TestRay){auto event=p;event.kind=p.kind==mcvalve::TestBlock?mcvalve::Block:mcvalve::Projectile;process(player,event);break;}
        if(p.kind==mcvalve::TestInput){
            CLIENT_COMMAND(player->edict(),"-forward;-back;-jump\n");testInputDeadline=0;
            if(p.i[0]>=1&&p.i[0]<=3){const char* commands[]={"","+forward\n","+back\n","+jump\n"};CLIENT_COMMAND(player->edict(),const_cast<char*>(commands[p.i[0]]));testInputDeadline=GetTickCount64()+DWORD(std::clamp(p.f[0],50.f,500.f));}
            ack(p,1);break;
        }
        if(p.kind==mcvalve::TestTeleport){
            Vector pos=start+Vector(0,0,36);UTIL_TraceHull(pos,pos,dont_ignore_monsters,human_hull,player->edict(),&tr);
            if((pos-player->pev->origin).Length()>256||tr.fStartSolid||tr.fAllSolid){ack(p,-1);break;}
            UTIL_SetOrigin(player->pev,pos);player->pev->velocity=g_vecZero;ack(p,1);break;
        }
        if(p.kind==mcvalve::TestInspect&&p.i[0]==99){mcvalve::Packet result;result.kind=mcvalve::Ack;result.sequence=p.sequence;result.i[0]=1;result.i[1]=(player->pev->flags&FL_NOTARGET)?1:0;result.f[0]=player->pev->health;link.send(result);break;}
        if(p.kind==mcvalve::TestInspect&&p.i[0]==98){mcvalve::Packet result;result.kind=mcvalve::Ack;result.sequence=p.sequence;result.i[0]=1;result.i[6]=player->pev->movetype;result.i[7]=movementOwned;Vector feet=player->pev->origin-Vector(0,0,36);mcvalve::toGuest(feet,result.f+2);link.send(result);break;}
        CBaseEntity* target=testTarget;
        if(p.kind==mcvalve::TestInspect&&p.i[0]>=1&&p.i[0]<=5){const char* names[]={"","monster_tentacle","monster_gargantua","monster_bigmomma","monster_nihilanth","monster_apache"};target=UTIL_FindEntityByClassname(nullptr,names[p.i[0]]);}
        if(p.kind==mcvalve::TestRemove){if(target)UTIL_Remove(target);testTarget=nullptr;ack(p,1);break;}
        if(p.kind==mcvalve::TestHealth){if(p.i[0]==1)player->TakeDamage(player->pev,player->pev,std::clamp(p.f[0],1.f,25.f),DMG_BURN);else player->pev->health=std::clamp(p.f[0],1.f,100.f);ack(p,1);break;}
        if(p.kind==mcvalve::TestAim){
            Vector delta=start-player->EyePosition();orient(player,Vector(-atan2f(delta.z,sqrtf(delta.x*delta.x+delta.y*delta.y))*57.2957795f,atan2f(delta.y,delta.x)*57.2957795f,0));ack(p,1);break;
        }
        if(p.kind==mcvalve::TestSpawn){
            if(target){UTIL_Remove(target);testTarget=nullptr;}
            if((start-player->pev->origin).Length()>256){ack(p,-1);break;}
            TraceResult ground;UTIL_TraceLine(start+Vector(0,0,96),start-Vector(0,0,128),ignore_monsters,player->edict(),&ground);
            if(ground.flFraction==1||ground.fStartSolid){ack(p,-1);break;}
            Vector foot=ground.vecEndPos+Vector(0,0,1),center=foot+Vector(0,0,36);
            UTIL_TraceHull(center,center,dont_ignore_monsters,human_hull,player->edict(),&tr);if(tr.fStartSolid||tr.fAllSolid){ack(p,-1);break;}
            target=CBaseEntity::Create("monster_zombie",foot,g_vecZero,nullptr);if(!target){ack(p,-1);break;}
            target->pev->health=1000;target->pev->max_health=1000;target->pev->movetype=MOVETYPE_NONE;target->SetThink(nullptr);target->pev->nextthink=0;UTIL_SetOrigin(target->pev,foot);testTarget=target;
        }
        mcvalve::Packet result;result.kind=mcvalve::Ack;result.sequence=p.sequence;result.i[0]=target?1:0;
        if(target){result.i[1]=ENTINDEX(target->edict());result.f[0]=target->pev->health;result.f[1]=target->pev->max_health;Vector center=target->Center();mcvalve::toGuest(center,result.f+2);}
        result.f[5]=gSkillData.plrDmgCrowbar;result.f[6]=gSkillData.plrDmg9MM;link.send(result);break;
    }
    case mcvalve::Strike:
        if(gpGlobals->time-lastMelee<0.45f){ack(p,0);break;}
        lastMelee=gpGlobals->time;start=player->EyePosition();UTIL_MakeVectors(player->pev->v_angle);
        UTIL_TraceLine(start,start+gpGlobals->v_forward*80,dont_ignore_monsters,player->edict(),&tr);
        if(tr.flFraction==1)UTIL_TraceHull(start,start+gpGlobals->v_forward*80,dont_ignore_monsters,head_hull,player->edict(),&tr);
        if(tr.flFraction<1)damage(player,tr,gSkillData.plrDmgCrowbar,DMG_CLUB);
        ack(p,tr.flFraction<1,tr.pHit?ENTINDEX(tr.pHit):0);break;
    case mcvalve::Projectile:
        if(impacts.count(p.i[0])){ack(p,0);break;}
        if((end-start).Length()>4096){ack(p,-1);break;}
        UTIL_TraceLine(start,end,dont_ignore_monsters,player->edict(),&tr);
        if(tr.flFraction<1){
            impacts[p.i[0]]=GetTickCount64();
            if(p.i[1]==5)RadiusDamage(tr.vecEndPos,player->pev,player->pev,gSkillData.plrDmgM203Grenade,192,CLASS_NONE,DMG_BLAST);
            else if(p.i[1]==3){
                Vector feet=tr.vecEndPos+tr.vecPlaneNormal*20;feet.z+=36;
                TraceResult hull;UTIL_TraceHull(feet,feet,dont_ignore_monsters,human_hull,player->edict(),&hull);
                if(!hull.fStartSolid&&!hull.fAllSolid){UTIL_SetOrigin(player->pev,feet);player->pev->velocity=g_vecZero;player->TakeDamage(player->pev,player->pev,25,DMG_FALL);}
            } else if(p.i[1]==1)damage(player,tr,30.f*std::clamp(p.f[6],0.1f,4.f),DMG_BULLET);
            {mcvalve::Packet out;out.kind=mcvalve::Ack;out.sequence=p.sequence;out.i[0]=1;out.i[1]=tr.pHit?ENTINDEX(tr.pHit):0;mcvalve::toGuest(tr.vecEndPos,out.f);link.send(out);}
        }else ack(p,0);
        break;
    case mcvalve::Blast:
        if((start-player->pev->origin).Length()>8192){ack(p,-1);break;}
        for(int n=0;n<16&&UTIL_PointContents(start)==CONTENTS_SOLID;n++)start.z+=4;
        if(UTIL_PointContents(start)==CONTENTS_SOLID){ack(p,-1);break;}
        RadiusDamage(start,player->pev,player->pev,gSkillData.plrDmgSatchel*std::clamp(p.f[6],0.1f,2.f),256,CLASS_NONE,DMG_BLAST);ack(p,1);break;
    case mcvalve::Block: {
        auto key=std::make_tuple(p.i[0],p.i[1],p.i[2]);auto found=blocks.find(key);
        if(!p.i[3]){if(found!=blocks.end()){if(found->second)UTIL_Remove(found->second);blocks.erase(found);}ack(p,1);break;}
        if(blocks.size()>=1024||found!=blocks.end()){ack(p,0);break;}
        float pos[3]={float(p.i[0])+.5f,float(p.i[1])+.5f,float(p.i[2])+.5f};Vector center=native(pos);
        if((center-player->pev->origin).Length()>256){ack(p,-1);break;}
        auto* entity=CBaseEntity::Create("info_target",center,g_vecZero,nullptr);
        if(!entity){ack(p,-1);break;}
        SET_MODEL(entity->edict(),"models/grenade.mdl");
        entity->pev->movetype=MOVETYPE_NONE;entity->pev->solid=SOLID_BBOX;entity->pev->rendermode=kRenderTransTexture;entity->pev->renderamt=0;
        UTIL_SetSize(entity->pev,Vector(-16,-16,-16),Vector(16,16,16));UTIL_SetOrigin(entity->pev,center);
        blocks[key]=entity;ack(p,1,ENTINDEX(entity->edict()));break;
    }
    case mcvalve::Health:
        // Native health is authoritative; only completed guest healing is forwarded.
        if(p.f[0]>=0)player->TakeHealth(std::clamp(p.f[0],0.f,100.f),DMG_GENERIC);
        else player->TakeDamage(player->pev,player->pev,std::clamp(-p.f[0],0.f,100.f),DMG_GENERIC);
        ack(p,1);break;
    case mcvalve::Effect:
        speed=std::clamp(p.f[0],1.f,2.f);jump=std::clamp(p.f[1],1.f,2.f);effectFlags=p.i[0]&7;effectTime=GetTickCount64();ack(p,1);break;
    case mcvalve::ChangeMap:
        if(p.i[0]<0||p.i[0]>=sizeof(mcvalve::halflifeMaps)/sizeof(*mcvalve::halflifeMaps)){ack(p,-1);break;}
        {char command[128];std::snprintf(command,sizeof(command),"map %s\nexec minecraftify.cfg\n",mcvalve::halflifeMaps[p.i[0]]);ack(p,1);SERVER_COMMAND(command);}break;
    case mcvalve::Travel: {
        if(p.i[0]>=10&&p.i[0]<=12){
            if(strcmp(STRING(gpGlobals->mapname),"c5a1")){ack(p,-1);break;}
            const char* actor=p.i[0]==10?"bridge_gman":p.i[0]==11?"end_gman":"loser_gman";
            const char* trigger=p.i[0]==10?"start_bridge_mm":p.i[0]==11?"v_black_room_sounds_mm1":"loser_mm";
            auto* speaker=UTIL_FindEntityByTargetname(nullptr,actor);
            auto* scene=UTIL_FindEntityByTargetname(nullptr,trigger);
            if(!speaker||!scene){ack(p,-3);break;}
            // Stop the opening schedule before entering a different ending branch.
            if(auto* opening=UTIL_FindEntityByTargetname(nullptr,"start_gman_mm"))opening->pev->nextthink=0;
            if(auto* opening=UTIL_FindEntityByTargetname(nullptr,"start_gman"))EMIT_SOUND_DYN(opening->edict(),CHAN_VOICE,"!GM_SUIT",0,ATTN_NONE,SND_STOP,PITCH_NORM);
            Vector where=p.i[0]==10?Vector(-1744,-2632,-2747):p.i[0]==11?Vector(-917,330,-2944):Vector(142,-2023,1349);
            UTIL_SetOrigin(player->pev,where);player->pev->velocity=g_vecZero;
            Vector delta=speaker->Center()-player->EyePosition();
            orient(player,Vector(-atan2f(delta.z,sqrtf(delta.x*delta.x+delta.y*delta.y))*57.2957795f,atan2f(delta.y,delta.x)*57.2957795f,0));
            FireTargets(trigger,player,player,USE_TOGGLE,0);ack(p,1);break;
        }
        const char* names[]={"","monster_tentacle","monster_gargantua","monster_bigmomma","monster_nihilanth","monster_apache"};
        if(p.i[0]<1||p.i[0]>5){ack(p,-1);break;}
        auto* boss=UTIL_FindEntityByClassname(nullptr,names[p.i[0]]);bool moved=false,hasFallback=false;Vector chosen,fallback;
        if(boss)for(int n=0;n<120&&!moved;n++){
            const float heights[]={64,128,256,512,1024};int spot=n%24;
            float angle=(spot%8)*.785398163f,distance=384.f+(spot/8)*128.f;
            Vector above=boss->pev->origin+Vector(cosf(angle)*distance,sinf(angle)*distance,heights[n/24]);
            TraceResult ground;UTIL_TraceLine(above,above-Vector(0,0,8192),ignore_monsters,player->edict(),&ground);
            if(ground.flFraction==1||ground.fStartSolid)continue;
            Vector target=ground.vecEndPos+Vector(0,0,37);TraceResult hull;UTIL_TraceHull(target,target,dont_ignore_monsters,human_hull,player->edict(),&hull);
            if(hull.fStartSolid||hull.fAllSolid)continue;
            if(!hasFallback){fallback=target;hasFallback=true;}
            TraceResult sight;UTIL_TraceLine(target+Vector(0,0,28),boss->Center(),dont_ignore_monsters,player->edict(),&sight);
            if(sight.flFraction<.99f&&sight.pHit!=boss->edict())continue;
            chosen=target;moved=true;
        }
        if(!moved&&hasFallback){chosen=fallback;moved=true;}
        if(moved){
            UTIL_SetOrigin(player->pev,chosen);player->pev->velocity=g_vecZero;
            Vector delta=boss->Center()-player->EyePosition();orient(player,Vector(-atan2f(delta.z,sqrtf(delta.x*delta.x+delta.y*delta.y))*57.2957795f,atan2f(delta.y,delta.x)*57.2957795f,0));moved=true;
        }
        ack(p,moved?1:-3);break;
    }
    default:ack(p,-2);break;
    }
}
}
void MinecraftifyServerFrame(){
    if(gpGlobals->maxClients!=1||!link.open(25611,25621))return;
    if(lastTime<0||gpGlobals->time<lastTime||strcmp(currentMap,STRING(gpGlobals->mapname))){
        blocks.clear();lastMelee=-1;link.scene=++sceneCounter+uint32_t(GetTickCount64());link.lastSequence=0;
        strncpy(currentMap,STRING(gpGlobals->mapname),63);terrainCursor=0;
        impacts.clear();
        testTarget=nullptr;movementOwned=false;guestMovement=false;movementFlags=0;
    }
    lastTime=gpGlobals->time;
    auto* player=dynamic_cast<CBasePlayer*>(UTIL_PlayerByIndex(1));if(!player)return;
    if(testInputDeadline&&(GetTickCount64()>testInputDeadline||!link.active())){CLIENT_COMMAND(player->edict(),"-forward;-back;-jump\n");testInputDeadline=0;}
    mcvalve::Packet p;
    for(int count=0;count<128&&link.receive(p);++count){
        if(p.kind==mcvalve::Heartbeat){if(p.scene==link.scene){if(!link.active()){link.lastSequence=0;impacts.clear();}link.lease=GetTickCount64();guestMovement=p.i[1]==1;}continue;}
        if(p.scene!=link.scene||!link.active()||p.sequence<=link.lastSequence)continue;
        link.lastSequence=p.sequence;
        if(player->IsAlive()||p.kind==mcvalve::TestInspect||p.kind==mcvalve::TestRemove||p.kind==mcvalve::Projectile||p.kind==mcvalve::Blast||p.kind==mcvalve::Block)process(player,p);
        else ack(p,0);
    }
    bool wantMovement=link.active()&&guestMovement&&player->IsAlive();
    if(wantMovement&&!movementOwned){previousMoveType=player->pev->movetype;movementOwned=true;movementGround=(player->pev->flags&FL_ONGROUND)!=0;player->pev->movetype=MOVETYPE_FLY;player->pev->velocity=g_vecZero;motion.clear();motionTime=GetTickCount64();}
    if(!wantMovement&&movementOwned){player->pev->movetype=previousMoveType;player->pev->velocity=g_vecZero;player->pev->view_ofs=Vector(0,0,28);movementOwned=false;movementFlags=0;motion.clear();}
    if(movementOwned){
        auto now=GetTickCount64();float dt=float(now-motionTime)/1000.f;motionTime=now;
        auto delta=motion.take(dt);mcvalve::Packet step;step.kind=mcvalve::Movement;step.i[0]=movementFlags;
        for(int n=0;n<3;n++)step.f[n]=delta[n];motionStep=true;process(player,step);motionStep=false;
    }

    if(movementOwned){player->pev->movetype=MOVETYPE_FLY;player->pev->velocity=g_vecZero;player->pev->fuser2=100+player->pev->view_ofs.z;}else player->pev->fuser2=0;
    if(!link.active()||GetTickCount64()-effectTime>1000){speed=jump=1;effectFlags=0;}
    if(effectFlags&2){if(!(player->pev->flags&FL_NOTARGET)){player->pev->flags|=FL_NOTARGET;ownedNoTarget=true;}}
    else if(ownedNoTarget){player->pev->flags&=~FL_NOTARGET;ownedNoTarget=false;}
    if(link.active()){
        player->pev->fuser1=speed;player->pev->maxspeed=320.f*speed;g_engfuncs.pfnSetClientMaxspeed(player->edict(),player->pev->maxspeed);player->pev->fuser4=jump;
        float feet[3]={player->pev->origin.x,player->pev->origin.y,player->pev->origin.z-36};float guest[3];mcvalve::toGuest(feet,guest);
        // Incremental local occupancy sampling; intentionally conservative whole blocks.
        if(GetTickCount64()-lastTerrain>50){lastTerrain=GetTickCount64();for(int count=0;count<48;count++){
            int n=terrainCursor++%729;int x=int(floorf(guest[0]))+n%9-4,y=int(floorf(guest[1]))+(n/9)%9-4,z=int(floorf(guest[2]))+n/81-4;
            float center[3]={float(x)+.5f,float(y)+.5f,float(z)+.5f};Vector pos=native(center);
            mcvalve::Packet cell;cell.kind=mcvalve::Terrain;cell.i[0]=x;cell.i[1]=y;cell.i[2]=z;cell.i[3]=UTIL_PointContents(pos)==CONTENTS_SOLID;link.send(cell);
        }}
    }else {player->pev->fuser1=0;player->pev->fuser4=1;player->pev->maxspeed=320;g_engfuncs.pfnSetClientMaxspeed(player->edict(),320);}
    for(auto at=impacts.begin();at!=impacts.end();)if(GetTickCount64()-at->second>10000)at=impacts.erase(at);else ++at;
    mcvalve::Packet state;state.kind=mcvalve::Vitals;state.f[0]=player->pev->health;state.f[1]=player->pev->max_health;
    state.i[0]=player->IsAlive();state.i[1]=int(blocks.size());link.send(state);
    mcvalve::Packet map;map.kind=mcvalve::Scene;strncpy(reinterpret_cast<char*>(map.f),STRING(gpGlobals->mapname),63);link.send(map);
}
float MinecraftifyPlayerDamage(float amount,int type){
    if(!link.active())return amount;
    if((effectFlags&1)&&(type&(DMG_BURN|DMG_SLOWBURN)))return 0;
    if(effectFlags&4)return amount*.8f;
    return amount;
}

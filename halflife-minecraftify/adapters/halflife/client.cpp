#include "../common/wire.hpp"
#include "../common/text_input.hpp"
#include "hud.h"
#include "cl_util.h"
#include "ref_params.h"
#include "usercmd.h"
#include "camera.h"
#include "Exports.h"
#include <SDL2/SDL.h>

namespace {
mcvalve::Link camera;
uint32_t frame=0,lastMask=0;
bool inventory=false,chat=false;
mcvalve::TextInput textInput;
bool playerAlive=true;
bool guestMovement=false;
bool key(int k){return (GetAsyncKeyState(k)&0x8001)!=0;}
}
bool MinecraftifyClientActive(){return camera.active();}
void MinecraftifyClientMove(usercmd_s* cmd){
    if(!camera.active())return;
    if(!playerAlive){if(key(VK_LBUTTON))cmd->buttons|=1;return;}
    cmd->buttons &= ~(1|2048); // native IN_ATTACK/IN_ATTACK2; Minecraft owns item use
    if(guestMovement){cmd->forwardmove=cmd->sidemove=cmd->upmove=0;cmd->buttons&=~(2|4);}
    if(inventory){cmd->forwardmove=cmd->sidemove=cmd->upmove=0;cmd->buttons=0;}
}
void MinecraftifyClientView(ref_params_s* p){
    extern float MinecraftifyNativeVerticalFov();
    if(p->maxclients!=1||!camera.open(25613,25621))return;
    mcvalve::Packet in;
    for(int n=0;n<16&&camera.receive(in);++n){
        if(in.kind==mcvalve::Heartbeat){camera.scene=in.scene;camera.lease=GetTickCount64();inventory=in.i[0]!=0;chat=in.i[2]==1;guestMovement=in.i[1]==1;}
        else if(in.kind==mcvalve::Ui&&in.i[0]==1&&camera.active()&&std::abs(in.f[0])<=90&&std::abs(in.f[1])<=360)gEngfuncs.SetViewAngles(in.f);
    }
    DWORD pid=0;HWND window=GetForegroundWindow();GetWindowThreadProcessId(window,&pid);
    bool focused=pid==GetCurrentProcessId();
    playerAlive=p->health>0;
    mcvalve::Packet pose;pose.kind=mcvalve::Camera;pose.sequence=++frame;
    mcvalve::toGuest(p->vieworg,pose.f);
    float feet[3]={p->simorg[0],p->simorg[1],p->simorg[2]-(guestMovement?36.f:(p->viewheight[2]<20?18.f:36.f))};
    mcvalve::toGuest(feet,pose.f+3);
    float eye[3]={p->simorg[0]+p->viewheight[0],p->simorg[1]+p->viewheight[1],p->simorg[2]+p->viewheight[2]};mcvalve::toGuest(eye,pose.f+10);
    pose.f[6]=180.f-p->viewangles[1];pose.f[7]=p->viewangles[0];pose.f[8]=-p->viewangles[2];
    float hfov=gHUD.m_iFOV>0?float(gHUD.m_iFOV):90.f;
    pose.f[9]=mcvalve::verticalFov(hfov,p->viewport[3]>0?float(p->viewport[2])/p->viewport[3]:16.f/9.f);
    float measuredFov=MinecraftifyNativeVerticalFov();if(measuredFov>=10&&measuredFov<=170)pose.f[9]=measuredFov;
    extern void MinecraftifyUpdateCompositePose(const float*,const float*,float,float);
    MinecraftifyUpdateCompositePose(pose.f,pose.f+6,pose.f[9],p->viewport[3]>0?float(p->viewport[2])/p->viewport[3]:16.f/9);
    pose.i[0]=focused&&!p->paused&&!p->intermission&&p->health>0;
    pose.i[1]=!cam_thirdperson;pose.i[2]=p->viewport[2];pose.i[3]=p->viewport[3];pose.i[4]=p->onground!=-1;pose.i[5]=!guestMovement&&p->viewheight[2]<20;pose.i[6]=guestMovement;camera.send(pose);
    uint32_t mask=0;
    const int keys[]={VK_LBUTTON,VK_RBUTTON,'I',VK_F5,VK_ESCAPE,'Q','F',VK_MBUTTON,VK_SHIFT,VK_CONTROL};
    for(int n=0;n<10;n++)if(focused&&key(keys[n]))mask|=1u<<n;
    for(int n=0;n<9;n++)if(focused&&key('1'+n))mask|=1u<<(10+n);
    if(focused&&key(VK_F8))mask|=1u<<20;
    if(focused&&key(VK_F6))mask|=1u<<21;
    if(focused&&key(VK_F9))mask|=1u<<27;
    if(focused&&key(VK_F10))mask|=1u<<28;
    if(focused&&key(VK_PRIOR))mask|=1u<<29;
    if(focused&&key(VK_NEXT))mask|=1u<<30;
    const int movementKeys[]={'W','S','A','D',VK_SPACE};
    for(int n=0;n<5;n++)if(focused&&key(movementKeys[n]))mask|=1u<<(22+n);
    if(focused&&key('T'))mask|=1u<<31;
    textInput.poll(camera,focused&&camera.active()&&chat);
    mcvalve::Packet input;input.kind=mcvalve::Input;input.i[0]=mask;input.i[1]=lastMask;
    RECT rect{};POINT cursor{};GetClientRect(window,&rect);GetCursorPos(&cursor);ScreenToClient(window,&cursor);
    input.f[0]=rect.right>0?float(cursor.x)/rect.right:0;input.f[1]=rect.bottom>0?float(cursor.y)/rect.bottom:0;
    camera.send(input);
    if(camera.active()&&focused&&!chat&&(mask&(1u<<3))&&!(lastMask&(1u<<3))){
        gEngfuncs.pfnClientCmd(cam_thirdperson?"firstperson\n":"thirdperson\n");
    }
    if(camera.active()&&inventory){IN_DeactivateMouse();}
    else if(camera.active()&&focused){IN_ActivateMouse();}
    SDL_bool capture=focused&&!p->paused&&!p->intermission&&(!camera.active()||!inventory)?SDL_TRUE:SDL_FALSE;
    if(SDL_GetRelativeMouseMode()!=capture)SDL_SetRelativeMouseMode(capture);
    lastMask=mask;
    if(camera.active()){
        auto* weapon=gEngfuncs.GetViewModel();if(weapon)weapon->curstate.effects|=128; // EF_NODRAW
    }
}
void MinecraftifyReleaseMouse(){SDL_SetRelativeMouseMode(SDL_FALSE);}

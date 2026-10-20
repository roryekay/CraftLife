#pragma once
#include <array>
#include <cmath>
#include <algorithm>
namespace fusion {
using Vec3=std::array<float,3>;
inline float dot3(Vec3 a,Vec3 b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
struct CameraPose {double x=0,y=0,z=0;float yaw=0,pitch=0,roll=0,fov=70,aspect=16.f/9;};
struct CameraBasis {Vec3 right,up,forward;};
inline CameraBasis basis(const CameraPose& p){
    constexpr float rad=.0174532925199433f;
    float y=p.yaw*rad,t=p.pitch*rad,r=p.roll*rad;
    float sy=std::sin(y),cy=std::cos(y),st=std::sin(t),ct=std::cos(t),sr=std::sin(r),cr=std::cos(r);
    Vec3 right={-cy,0,-sy},up={-sy*st,ct,cy*st};CameraBasis b;
    for(int i=0;i<3;i++){b.right[i]=right[i]*cr+up[i]*sr;b.up[i]=up[i]*cr-right[i]*sr;}
    b.forward={-sy*ct,-st,cy*ct};return b;
}
// Current screen ray -> captured camera ray. Rows map (2u-1,1-2v,1)
// to old normalized camera coordinates; divide XY by Z before sampling.
struct Reprojection {Vec3 x{1,0,0},y{0,1,0},z{0,0,1},offset{};};
inline Reprojection reproject(const CameraPose& captured,const CameraPose& current){
    auto old=basis(captured),now=basis(current);
    float a=std::tan(captured.fov*.00872664626f),b=std::tan(current.fov*.00872664626f);
    Reprojection m;Vec3 columns[]={now.right,now.up,now.forward};
    float scale[]={b*current.aspect,b,1};
    for(int i=0;i<3;i++){m.x[i]=dot3(old.right,columns[i])*scale[i]/(a*captured.aspect);m.y[i]=dot3(old.up,columns[i])*scale[i]/a;m.z[i]=dot3(old.forward,columns[i])*scale[i];}
    Vec3 delta={float(current.x-captured.x),float(current.y-captured.y),float(current.z-captured.z)};
    m.offset={dot3(old.right,delta)/(a*captured.aspect),dot3(old.up,delta)/a,dot3(old.forward,delta)};
    return m;
}
inline Vec3 project(const CameraPose& camera,Vec3 world){
    auto b=basis(camera);Vec3 d={world[0]-float(camera.x),world[1]-float(camera.y),world[2]-float(camera.z)};
    float z=dot3(d,b.forward),t=std::tan(camera.fov*.00872664626f);
    return {.5f+.5f*dot3(d,b.right)/(z*t*camera.aspect),.5f-.5f*dot3(d,b.up)/(z*t),z};
}
}

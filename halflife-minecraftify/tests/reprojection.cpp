#include <fusion/reprojection.hpp>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
int main(){
    using namespace fusion;CameraPose old;old.fov=78;CameraPose now;now.fov=70;
    for(float yaw:{-25.f,0.f,25.f,179.f,-179.f})for(float pitch:{-35.f,0.f,35.f}){
        old.yaw=yaw;old.pitch=pitch;old.roll=3;now=old;now.yaw+=12;now.pitch-=7;now.fov=70;
        auto b=basis(now);Vec3 world;for(int i=0;i<3;i++)world[i]=b.forward[i]*10+b.right[i]*2+b.up[i];
        auto a=project(old,world),screen=project(now,world);auto m=reproject(old,now);Vec3 q={screen[0]*2-1,1-screen[1]*2,1};
        float z=dot3(m.z,q);assert(std::abs((.5f+.5f*dot3(m.x,q)/z)-a[0])<.00001f);assert(std::abs((.5f-.5f*dot3(m.y,q)/z)-a[1])<.00001f);
    }
    old={0,1,0,23,11,0,78,16.f/9};now={.2,1.1,-.3,28,8,0,70,16.f/9};
    auto b=basis(now);Vec3 point;for(int i=0;i<3;i++)point[i]=float(i==0?now.x:i==1?now.y:now.z)+b.forward[i]*12+b.right[i];
    auto a=project(old,point),current=project(now,point);auto warp=reproject(old,now);Vec3 q={current[0]*2-1,1-current[1]*2,1};
    float currentDepth=(a[2]-warp.offset[2])/dot3(warp.z,q);
    assert(std::abs(currentDepth-current[2])<.0001f);
    assert(std::abs(.5f+.5f*(dot3(warp.x,q)*currentDepth+warp.offset[0])/a[2]-a[0])<.00001f);
    assert(std::abs(.5f-.5f*(dot3(warp.y,q)*currentDepth+warp.offset[1])/a[2]-a[1])<.00001f);
    std::cout<<"PASS: captured/current camera ray reprojection across yaw wrap, pitch, roll and overscan FOV\n";
}

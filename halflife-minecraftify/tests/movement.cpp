#include "../adapters/common/movement.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
using namespace mcvalve;
int main(){
    MotionQueue queue;queue.push({1,2,-3});MoveVec total{};
    for(int frame=0;frame<5;frame++){auto d=queue.take(.01f);for(int n=0;n<3;n++)total[n]+=d[n];assert(std::abs(d[0]-.2f)<.0001f);}
    assert(std::abs(total[0]-1)<.0001f&&std::abs(total[1]-2)<.0001f&&std::abs(total[2]+3)<.0001f);
    assert(queue.take(.05f)[0]==0);queue.push({5,0,0});queue.clear();assert(queue.take(.05f)[0]==0);
    auto trace=[](MoveVec a,MoveVec b){
        MoveHit h;if(a[2]<-.001f||a[0]>4.001f||a[2]>5.001f){h.solid=true;return h;}
        auto plane=[&](int axis,float boundary,MoveVec normal){float d=b[axis]-a[axis];if(std::abs(d)<.00001f)return;float t=(boundary-a[axis])/d;if(t>=0&&t<h.fraction){h.fraction=t;h.normal=normal;}};
        if(b[0]>4)plane(0,4,{-1,0,0});if(b[2]<0)plane(2,0,{0,0,1});if(b[2]>5)plane(2,5,{0,0,-1});return h;
    };
    auto wall=move({0,0,1},{400,10,1},trace,false,true);assert(wall.feet[0]<4.001f&&wall.feet[1]>9.99f&&(wall.blocked&4));
    auto floor=move({0,0,1},{0,0,-100},trace,false,false);assert(floor.feet[2]>=0&&floor.feet[2]<.04f&&floor.grounded&&(floor.blocked&2));
    auto ceiling=move({0,0,1},{0,0,20},trace,false,true);assert(ceiling.feet[2]<5&&ceiling.feet[2]>4.9f);
    auto stuck=move({0,0,-1},{0,0,0},trace,false,false);assert(stuck.blocked==7&&stuck.feet[2]==-1);
    auto stair=[](MoveVec a,MoveVec b){MoveHit h;
        if(b[2]<0){h.fraction=a[2]/(a[2]-b[2]);h.normal={0,0,1};}
        if(a[0]<2&&b[0]>2&&a[2]<16){float f=(2-a[0])/(b[0]-a[0]);if(f<h.fraction){h.fraction=f;h.normal={-1,0,0};}}
        return h;
    };
    auto step=move({0,0,.03125f},{6,0,-.05f},stair,true,false);assert(step.feet[0]>5.99f&&step.grounded);
    auto flying=move({0,0,.03125f},{6,0,-.05f},stair,true,true);assert(flying.feet[0]<2);
    std::cout<<"PASS: high-speed sweep, wall slide, floor, ceiling, grounded step, glide step refusal and solid-start refusal\n";
}

#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <deque>
namespace mcvalve {
using MoveVec=std::array<float,3>;
// Spread each Minecraft tick over native frames without changing its distance.
class MotionQueue {
    struct Slice{MoveVec delta;float seconds;};
    std::deque<Slice> slices;
public:
    void clear(){slices.clear();}
    void push(MoveVec delta){if(slices.size()>=4)slices.pop_front();slices.push_back({delta,.05f});}
    MoveVec take(float seconds){
        MoveVec out{};seconds=std::clamp(seconds,0.f,.05f);
        while(seconds>0&&!slices.empty()){
            auto& s=slices.front();float dt=std::min(seconds,s.seconds),fraction=dt/s.seconds;
            for(int n=0;n<3;n++){float d=s.delta[n]*fraction;out[n]+=d;s.delta[n]-=d;}
            seconds-=dt;s.seconds-=dt;if(s.seconds<.000001f)slices.pop_front();
        }
        return out;
    }
};
inline MoveVec add(MoveVec a,MoveVec b){for(int i=0;i<3;i++)a[i]+=b[i];return a;}
inline MoveVec sub(MoveVec a,MoveVec b){for(int i=0;i<3;i++)a[i]-=b[i];return a;}
inline MoveVec mul(MoveVec a,float s){for(float& v:a)v*=s;return a;}
inline float dot(MoveVec a,MoveVec b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
struct MoveHit{float fraction=1;MoveVec normal{};bool solid=false;};
struct MoveResult{MoveVec feet{};int blocked=0;bool grounded=false;};
// Engine hull sweeps own collision. Minecraft supplies its actual desired motion.
template<class Trace> MoveResult slide(MoveVec from,MoveVec delta,Trace trace){
    MoveResult out;out.feet=from;
    for(int bump=0;bump<4&&dot(delta,delta)>.000001f;bump++){
        auto hit=trace(out.feet,add(out.feet,delta));
        if(hit.solid){out.blocked|=7;break;}
        float f=std::clamp(hit.fraction,0.f,1.f);
        out.feet=add(out.feet,mul(delta,f));
        if(f>=1)break;
        out.feet=add(out.feet,mul(hit.normal,.03125f));
        // Wire bits use Minecraft X/Y/Z, while these vectors use native X/Y/Z.
        if(std::abs(hit.normal[0])>.01f)out.blocked|=4;
        if(std::abs(hit.normal[1])>.01f)out.blocked|=1;
        if(std::abs(hit.normal[2])>.01f)out.blocked|=2;
        delta=mul(delta,1-f);float into=dot(delta,hit.normal);
        if(into<0)delta=sub(delta,mul(hit.normal,into));
    }
    return out;
}
template<class Trace> MoveResult move(MoveVec from,MoveVec to,Trace trace,bool wasGrounded,bool gliding){
    auto delta=sub(to,from);auto out=slide(from,delta,trace);
    if(wasGrounded&&!gliding&&(out.blocked&5)&&delta[2]<=.1f){
        MoveVec up=add(from,{0,0,19.2f});auto rise=trace(from,up);
        if(!rise.solid&&rise.fraction==1){
            auto stepped=slide(up,{delta[0],delta[1],0},trace);
            auto down=trace(stepped.feet,add(stepped.feet,{0,0,-21.2f}));
            if(!down.solid&&down.fraction<1&&down.normal[2]>.6f){
                stepped.feet=add(stepped.feet,{0,0,-21.2f*down.fraction+.03125f});
                auto a=sub(out.feet,from),b=sub(stepped.feet,from);
                if(b[0]*b[0]+b[1]*b[1]>a[0]*a[0]+a[1]*a[1]+.01f){stepped.blocked|=2;out=stepped;}
            }
        }
    }
    auto floor=trace(out.feet,add(out.feet,{0,0,-2}));
    out.grounded=delta[2]<=.01f&&!floor.solid&&floor.fraction<1&&floor.normal[2]>.6f;
    return out;
}
}

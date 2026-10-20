#include "../adapters/common/wire.hpp"
#undef NDEBUG
#include <cassert>
#include <cstddef>
#include <limits>
int main(){
    using namespace mcvalve;
    static_assert(offsetof(Packet,f)==16&&offsetof(Packet,i)==96);
    float original[3]={1024,-73,128},guest[3],restored[3];toGuest(original,guest);toHost(guest,restored);
    for(int n=0;n<3;n++)assert(std::abs(original[n]-restored[n])<.0001f);
    assert(guest[0]==73.f/32&&guest[1]==68&&guest[2]==-32);
    // Native forward +X, left +Y, up +Z must remain right handed.
    float origin[3]={0,0,0},x[3]={32,0,0},y[3]={0,32,0},z[3]={0,0,32};
    float o[3],a[3],b[3],c[3];toGuest(origin,o);toGuest(x,a);toGuest(y,b);toGuest(z,c);
    for(int n=0;n<3;n++){a[n]-=o[n];b[n]-=o[n];c[n]-=o[n];}
    assert((a[1]*b[2]-a[2]*b[1])*c[0]+(a[2]*b[0]-a[0]*b[2])*c[1]+(a[0]*b[1]-a[1]*b[0])*c[2]>0);
    assert(std::abs(verticalFov(90,16.f/9)-58.7155)<.001);
    Packet p;assert(valid(p));p.f[0]=std::numeric_limits<float>::quiet_NaN();assert(!valid(p));
    p.f[0]=0;p.version=2;assert(!valid(p));
}

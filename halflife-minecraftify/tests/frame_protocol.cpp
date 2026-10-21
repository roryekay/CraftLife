#include <fusion/frame_protocol.hpp>
#include "../adapters/stub/adapter.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
template<class T> void put(uint8_t* p,int offset,T v){std::memcpy(p+offset,&v,sizeof v);}
void require(bool ok,const char* label){if(!ok)throw std::runtime_error(label);}
int main() {
    std::array<uint8_t,4096> h{};auto* p=h.data();
    put(p,0,fusion::magic);put(p,4,fusion::version);put(p,8,fusion::header_size);put(p,12,fusion::slots);
    put(p,16,fusion::slot_stride);put(p,24,fusion::max_width);put(p,28,fusion::max_height);
    require(fusion::valid_header(p),"valid header refused");put(p,16,uint64_t(16));require(!fusion::valid_header(p),"incompatible stride accepted");
    std::array<uint8_t,128> d{};p=d.data();put(p,0,uint64_t(2));put(p,8,uint64_t(1));put(p,24,uint32_t(1280));put(p,28,uint32_t(720));put(p,32,.05f);put(p,36,2048.f);put(p,40,70.f);put(p,44,uint32_t(7));
    fusion::Frame f;require(fusion::descriptor(p,f),"valid capture refused");
    put(p,0,uint64_t(3));require(!fusion::descriptor(p,f),"in-progress capture accepted");put(p,0,uint64_t(4));
    put(p,24,uint32_t(0xffffffff));require(!fusion::descriptor(p,f),"oversized capture accepted");put(p,24,uint32_t(1280));
    put(p,32,std::numeric_limits<float>::quiet_NaN());require(!fusion::descriptor(p,f),"NaN planes accepted");put(p,32,.05f);
    put(p,48,std::numeric_limits<double>::infinity());require(!fusion::descriptor(p,f),"infinite camera accepted");put(p,48,0.0);
    put(p,44,uint32_t(15));require(fusion::descriptor(p,f),"native foreground capture refused");
    put(p,44,uint32_t(16));require(!fusion::descriptor(p,f),"unknown flags accepted");
    minecraftify::BlankAdapter stub;minecraftify::Camera cam;require(!stub.camera(cam),"blank adapter enabled without host");
    std::cout<<"PASS: frame bounds, layout, incomplete writes, finite metadata and inactive stub\n";
}

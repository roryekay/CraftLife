// MIT. Wire layout derived from Rehan's universal-modder frame exporter.
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace fusion {
constexpr uint32_t magic=0x5450434d,version=1,header_size=4096,slots=3,max_width=3840,max_height=2160;
constexpr uint64_t slot_stride=uint64_t(max_width)*max_height*12,mapping_size=header_size+slots*slot_stride;
template<class T> T read(const uint8_t* p) { T out;std::memcpy(&out,p,sizeof out);return out; }
struct Frame {
    uint64_t capture{},host_frame{},sequence{};
    uint32_t width{},height{},flags{};
    float near_clip{},far_clip{},fov{},yaw{},pitch{},roll{};
    double x{},y{},z{};
    std::vector<uint8_t> pixels;
};
inline bool valid_header(const uint8_t* p) {
    return read<uint32_t>(p)==magic&&read<uint32_t>(p+4)==version&&read<uint32_t>(p+8)==header_size&&
        read<uint32_t>(p+12)==slots&&read<uint64_t>(p+16)==slot_stride&&read<uint32_t>(p+24)==max_width&&read<uint32_t>(p+28)==max_height;
}
inline bool descriptor(const uint8_t* d,Frame& f) {
    f.sequence=read<uint64_t>(d);f.capture=read<uint64_t>(d+8);f.host_frame=read<uint64_t>(d+16);
    f.width=read<uint32_t>(d+24);f.height=read<uint32_t>(d+28);
    f.near_clip=read<float>(d+32);f.far_clip=read<float>(d+36);f.fov=read<float>(d+40);f.flags=read<uint32_t>(d+44);
    f.x=read<double>(d+48);f.y=read<double>(d+56);f.z=read<double>(d+64);
    f.yaw=read<float>(d+72);f.pitch=read<float>(d+76);f.roll=read<float>(d+80);
    return !(f.sequence&1)&&f.capture>0&&f.width>0&&f.height>0&&f.width<=max_width&&f.height<=max_height&&
        uint64_t(f.width)*f.height*12<=slot_stride&&std::isfinite(f.near_clip)&&std::isfinite(f.far_clip)&&
        f.near_clip>0&&f.far_clip>f.near_clip&&std::isfinite(f.fov)&&f.fov>=10&&f.fov<=170&&
        std::isfinite(f.x)&&std::isfinite(f.y)&&std::isfinite(f.z)&&std::isfinite(f.yaw)&&std::isfinite(f.pitch)&&std::isfinite(f.roll)&&!(f.flags&~7u);
}
}

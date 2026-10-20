#pragma once
#include "reprojection.hpp"
#include <windows.h>
#include <cstring>
#include <cstdint>
namespace fusion {
struct PoseRecord {uint32_t magic=0,version=0,bytes=0,pid=0;LONG sequence=0;uint32_t reserved=0;uint64_t published=0;float p[3]{},yaw=0,pitch=0,roll=0,fov=0,aspect=0;};
static_assert(sizeof(PoseRecord)==64,"pose channel layout");
constexpr uint32_t poseMagic=0x3150534d;
class PoseWriter {
    HANDLE mapping=nullptr;PoseRecord* view=nullptr;
public:
    ~PoseWriter(){if(view)UnmapViewOfFile(view);if(mapping)CloseHandle(mapping);}
    void publish(const CameraPose& p){
        if(!view){mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(PoseRecord),L"Local\\MinecraftifyAnythingPose");if(!mapping)return;view=static_cast<PoseRecord*>(MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,sizeof(PoseRecord)));if(!view){CloseHandle(mapping);mapping=nullptr;return;}}
        LONG seq=view->sequence;if(seq&1)seq++;
        InterlockedExchange(&view->sequence,seq+1);MemoryBarrier();
        view->magic=poseMagic;view->version=1;view->bytes=sizeof(PoseRecord);view->pid=GetCurrentProcessId();view->published=GetTickCount64();
        view->p[0]=float(p.x);view->p[1]=float(p.y);view->p[2]=float(p.z);view->yaw=p.yaw;view->pitch=p.pitch;view->roll=p.roll;view->fov=p.fov;view->aspect=p.aspect;
        MemoryBarrier();InterlockedExchange(&view->sequence,seq+2);
    }
};
class PoseReader {
    HANDLE mapping=nullptr;const PoseRecord* view=nullptr;ULONGLONG nextOpen=0;
public:
    ~PoseReader(){if(view)UnmapViewOfFile(view);if(mapping)CloseHandle(mapping);}
    bool latest(CameraPose& p){
        if(!view){if(GetTickCount64()<nextOpen)return false;nextOpen=GetTickCount64()+250;mapping=OpenFileMappingW(FILE_MAP_READ,FALSE,L"Local\\MinecraftifyAnythingPose");if(!mapping)return false;view=static_cast<const PoseRecord*>(MapViewOfFile(mapping,FILE_MAP_READ,0,0,sizeof(PoseRecord)));if(!view){CloseHandle(mapping);mapping=nullptr;return false;}}
        LONG seq=view->sequence;if(seq&1)return false;MemoryBarrier();PoseRecord copy;std::memcpy(&copy,view,sizeof(copy));MemoryBarrier();
        if(seq!=view->sequence||copy.magic!=poseMagic||copy.version!=1||copy.bytes!=sizeof(copy)||GetTickCount64()-copy.published>250)return false;
        const float values[]={copy.p[0],copy.p[1],copy.p[2],copy.yaw,copy.pitch,copy.roll,copy.fov,copy.aspect};for(float v:values)if(!std::isfinite(v)||std::abs(v)>3e7)return false;
        if(copy.fov<10||copy.fov>170||copy.aspect<.1f||copy.aspect>10)return false;
        p={copy.p[0],copy.p[1],copy.p[2],copy.yaw,copy.pitch,copy.roll,copy.fov,copy.aspect};return true;
    }
};
}

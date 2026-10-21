#pragma once
#include <cstdint>
#include <cmath>
#include <cstring>
#include <algorithm>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <windows.h>
#endif

namespace mcvalve {
constexpr uint32_t magic=0x31564d43;
enum Kind : uint16_t { Camera=1, Vitals=2, Input=3, Scene=4, Terrain=5,
    Heartbeat=9, Strike=10, Projectile=11, Blast=12, Pearl=13,
    Block=14, Health=15, Effect=16, Ui=17, Travel=18, ChangeMap=19,
    TestSpawn=20,TestInspect=21,TestRemove=22,TestAim=23,TestHealth=24,
    TestInput=25,TestTeleport=26,TestBlock=27,TestRay=28,TestPlatform=29,Movement=30,MovementResult=31,Ending=33,FinishEnding=34,Ack=100 };
// Explicit fixed-width little-endian IPC. Never send pointers or engine structs.
struct Packet {
    uint32_t magicValue=magic;
    uint16_t version=1,kind=0;
    uint32_t sequence=0,scene=0;
    float f[20]{};
    int32_t i[8]{};
};
static_assert(sizeof(Packet)==128,"wire layout");
inline bool valid(const Packet& p) {
    if(p.magicValue!=magic||p.version!=1)return false;
    for(float v:p.f)if(!std::isfinite(v)||std::abs(v)>30000000.f)return false;
    return true;
}
// Valve +X forward,+Y left,+Z up -> Minecraft -Z,-X,+Y. One block=32 inches.
constexpr float scale=32.f;
inline void toGuest(const float* v,float* o){o[0]=-v[1]/scale;o[1]=v[2]/scale+64;o[2]=-v[0]/scale;}
inline void toHost(const float* v,float* o){o[0]=-v[2]*scale;o[1]=-v[0]*scale;o[2]=(v[1]-64)*scale;}
inline float verticalFov(float horizontal,float aspect){return 2.f*std::atan(std::tan(horizontal*0.00872664626f)/aspect)*57.2957795f;}
#ifdef _WIN32
class Link {
    SOCKET socket_=INVALID_SOCKET;
    sockaddr_in relay_{};
public:
    uint32_t scene=0,lastSequence=0;
    ULONGLONG lease=0;
    int receivedKind=0,receivedPort=0,receivedSize=0,receivedValid=0;
    ~Link(){if(socket_!=INVALID_SOCKET)closesocket(socket_);}
    bool open(unsigned short local,unsigned short relay){
        if(socket_!=INVALID_SOCKET)return true;
        WSADATA w{};if(WSAStartup(MAKEWORD(2,2),&w))return false;
        socket_=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(socket_==INVALID_SOCKET)return false;
        sockaddr_in at{};at.sin_family=AF_INET;at.sin_addr.s_addr=htonl(INADDR_LOOPBACK);at.sin_port=htons(local);
        if(bind(socket_,reinterpret_cast<sockaddr*>(&at),sizeof(at))){closesocket(socket_);socket_=INVALID_SOCKET;return false;}
        u_long nonblocking=1;ioctlsocket(socket_,FIONBIO,&nonblocking);
        // A missing relay is normal during startup/reconnect. Windows otherwise
        // turns ICMP port-unreachable replies into a backlog of WSAECONNRESETs,
        // starving the game-thread receiver of subsequent real control packets.
        BOOL udpReset=FALSE;DWORD returned=0;
        constexpr DWORD udpConnReset=_WSAIOW(IOC_VENDOR,12);
        if(WSAIoctl(socket_,udpConnReset,&udpReset,sizeof(udpReset),nullptr,0,&returned,nullptr,nullptr)){
            closesocket(socket_);socket_=INVALID_SOCKET;return false;
        }
        relay_=at;relay_.sin_port=htons(relay);return true;
    }
    void send(Packet p){if(socket_==INVALID_SOCKET)return;p.scene=scene;sendto(socket_,reinterpret_cast<const char*>(&p),sizeof(p),0,reinterpret_cast<sockaddr*>(&relay_),sizeof(relay_));}
    bool receive(Packet& p){
        if(socket_==INVALID_SOCKET)return false;
        sockaddr_in from{};int n=sizeof(from);
        int len=recvfrom(socket_,reinterpret_cast<char*>(&p),sizeof(p),0,reinterpret_cast<sockaddr*>(&from),&n);
        if(len>=0){receivedKind=p.kind;receivedPort=ntohs(from.sin_port);receivedSize=len;receivedValid=valid(p)?1:0;}
        else {int error=WSAGetLastError();if(error!=WSAEWOULDBLOCK)receivedSize=-error;}
        return len==sizeof(p)&&from.sin_addr.s_addr==htonl(INADDR_LOOPBACK)&&from.sin_port==relay_.sin_port&&valid(p);
    }
    bool active()const{return lease&&GetTickCount64()-lease<500;}
};
#endif
}

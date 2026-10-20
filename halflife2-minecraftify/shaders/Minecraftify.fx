#include "ReShade.fxh"
texture World : MCWORLD;
texture Depth : MCDEPTH;
texture Overlay : MCOVERLAY;
sampler sWorld { Texture=World; AddressU=CLAMP; AddressV=CLAMP; };
sampler sDepth { Texture=Depth; MinFilter=POINT; MagFilter=POINT; AddressU=CLAMP; AddressV=CLAMP; };
sampler sOverlay { Texture=Overlay; AddressU=CLAMP; AddressV=CLAMP; };
uniform bool McActive=false;
uniform float3 McPlanes=float3(.05,2048,7);
uniform float2 HostPlanes=float2(.1,10000);
uniform bool DepthGate < ui_label="Enable calibrated native depth occlusion"; > = false;
uniform bool HostReversedZ=false;
uniform float DepthBias=.04;
uniform float3 McCursor=float3(0,0,0);
float linearMc(float d){
    int flags=(int)McPlanes.z; if((flags&4)!=0)d=1-d;
    return McPlanes.x*McPlanes.y/max(.00001,McPlanes.y-d*(McPlanes.y-McPlanes.x));
}
float4 composite(float4 pos:SV_Position,float2 uv:TEXCOORD):SV_Target{
    float4 host=tex2D(ReShade::BackBuffer,uv);if(!McActive)return host;
    float2 mcuv=uv;if(((int)McPlanes.z&2)!=0)mcuv.y=1-mcuv.y;
    float4 world=tex2D(sWorld,mcuv),overlay=tex2D(sOverlay,mcuv);
    float nativeD=tex2D(ReShade::DepthBuffer,uv).x;if(HostReversedZ)nativeD=1-nativeD;
    float nativeZ=HostPlanes.x*HostPlanes.y/max(.00001,HostPlanes.y-nativeD*(HostPlanes.y-HostPlanes.x));
    if(DepthGate&&linearMc(tex2D(sDepth,mcuv).x)>nativeZ+DepthBias)world=0;
    float3 color=world.rgb+host.rgb*(1-world.a);
    color=overlay.rgb+color*(1-overlay.a);
    float2 delta=(uv-McCursor.xy)*float2(BUFFER_WIDTH,BUFFER_HEIGHT);
    if(McCursor.z>0&&((abs(delta.x)<1&&abs(delta.y)<7)||(abs(delta.y)<1&&abs(delta.x)<7)))color=1-color;
    return float4(color,1);
}
technique Minecraftify { pass { VertexShader=PostProcessVS; PixelShader=composite; } }

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
uniform float3 WarpX=float3(1,0,0),WarpY=float3(0,1,0),WarpZ=float3(0,0,1),WarpOffset=float3(0,0,0);
float2 storageUv(float2 uv){if(floor(McPlanes.z/2)-2*floor(McPlanes.z/4)>.5)uv.y=1-uv.y;return uv;}
float linearMc(float d){
    if(floor(McPlanes.z/4)-2*floor(McPlanes.z/8)>0.5)d=1-d;
    return McPlanes.x*McPlanes.y/max(.00001,McPlanes.y-d*(McPlanes.y-McPlanes.x));
}
float4 composite(float4 pos:SV_Position,float2 uv:TEXCOORD):SV_Target{
    float4 host=tex2D(ReShade::BackBuffer,uv);if(!McActive)return host;
    float3 q=float3(uv.x*2-1,1-uv.y*2,1),ray=float3(dot(WarpX,q),dot(WarpY,q),dot(WarpZ,q));
    float2 oldUv=.5+.5*float2(ray.x,-ray.y)/max(.00001,ray.z);float currentZ=0;
    for(int n=0;n<4;n++){float z=linearMc(tex2D(sDepth,storageUv(oldUv)).x);currentZ=(z-WarpOffset.z)/max(.00001,ray.z);oldUv=.5+.5*float2(ray.x*currentZ+WarpOffset.x,-ray.y*currentZ-WarpOffset.y)/max(.00001,z);}
    float4 world=tex2D(sWorld,storageUv(oldUv)),overlay=tex2D(sOverlay,storageUv(uv));
    if(ray.z<=0||currentZ<=0||any(oldUv<0)||any(oldUv>1))world=0;
    float nativeD=tex2D(ReShade::DepthBuffer,uv).x;if(HostReversedZ)nativeD=1-nativeD;
    float nativeZ=HostPlanes.x*HostPlanes.y/max(.00001,HostPlanes.y-nativeD*(HostPlanes.y-HostPlanes.x));
    if(DepthGate&&currentZ>nativeZ+DepthBias)world=0;
    // A native foreground can stand in a guest landscape. Source reports depth
    // in inches (32 per block); this mode uses its supported default clip planes.
    bool foreground=floor(McPlanes.z/8)-2*floor(McPlanes.z/16)>.5;
    if(foreground){
        float actorZ=(3.0*32768.0/max(.00001,32768.0-nativeD*(32768.0-3.0)))/32.0;
        float mask=smoothstep(.001,.006,max(host.r,max(host.g,host.b)));
        // Some D3D9 drivers do not expose actor depth. The ending stage has a
        // deliberately cleared black backdrop, allowing a bounded fallback.
        if(nativeD>.000001&&nativeD<.999999&&actorZ>currentZ+.04)mask=0;
        world*=1-mask;
    }
    float3 color=world.rgb+host.rgb*(1-world.a);
    color=overlay.rgb+color*(1-overlay.a);
    float2 delta=(uv-McCursor.xy)*float2(BUFFER_WIDTH,BUFFER_HEIGHT);
    if(McCursor.z>0&&((abs(delta.x)<1&&abs(delta.y)<7)||(abs(delta.y)<1&&abs(delta.x)<7)))color=1-color;
    return float4(color,1);
}
technique Minecraftify { pass { VertexShader=PostProcessVS; PixelShader=composite; } }

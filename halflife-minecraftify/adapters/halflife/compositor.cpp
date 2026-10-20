// GoldSrc's documented client draw callbacks composite the template's real frames.
// MIT adapter code. Engine SDK remains under Valve's separate SDK license.
#include "../common/wire.hpp"
#include <fusion/frame_reader.hpp>
#include <fusion/reprojection.hpp>
#include <GL/gl.h>
#include "hud.h"
#include "cl_util.h"
#include <array>

namespace {
using Char=char;
using CreateShaderFn=GLuint(APIENTRY*)(GLenum);
using ShaderSourceFn=void(APIENTRY*)(GLuint,GLsizei,const Char* const*,const GLint*);
using ShaderFn=void(APIENTRY*)(GLuint);
using GetObjectFn=void(APIENTRY*)(GLuint,GLenum,GLint*);
using InfoLogFn=void(APIENTRY*)(GLuint,GLsizei,GLsizei*,Char*);
using CreateProgramFn=GLuint(APIENTRY*)();
using AttachFn=void(APIENTRY*)(GLuint,GLuint);
using LocationFn=GLint(APIENTRY*)(GLuint,const Char*);
using Uniform1iFn=void(APIENTRY*)(GLint,GLint);
using Uniform4fFn=void(APIENTRY*)(GLint,GLfloat,GLfloat,GLfloat,GLfloat);
using ActiveTextureFn=void(APIENTRY*)(GLenum);
CreateShaderFn createShader;ShaderSourceFn shaderSource;ShaderFn compileShader,deleteShader,useProgram,linkProgram;
GetObjectFn getShader,getProgram;InfoLogFn shaderLog,programLog;CreateProgramFn createProgram;AttachFn attachShader;
LocationFn location;Uniform1iFn uniform1i;Uniform4fFn uniform4f;ActiveTextureFn activeTexture;
fusion::FrameReader reader;fusion::Frame frame;
HGLRC context=nullptr;GLuint program=0,textures[4]{};
int width=0,height=0,depthWidth=0,depthHeight=0;
ULONGLONG received=0;
bool failed=false,projectionValid=false;
float projectionA=0,projectionB=0;
float verticalFov=0;
fusion::CameraPose livePose;bool havePose=false;
template<class T> bool load(T& fn,const char* name){fn=reinterpret_cast<T>(wglGetProcAddress(name));return fn!=nullptr&&reinterpret_cast<uintptr_t>(fn)>3&&reinterpret_cast<intptr_t>(fn)!=-1;}
void message(const char* text){gEngfuncs.Con_Printf("Minecraftify OpenGL: %s\n",text);}
bool initialize(){
    bool ok=load(createShader,"glCreateShader")&&load(shaderSource,"glShaderSource")&&load(compileShader,"glCompileShader")&&load(getShader,"glGetShaderiv")&&load(shaderLog,"glGetShaderInfoLog")&&load(deleteShader,"glDeleteShader")&&load(createProgram,"glCreateProgram")&&load(attachShader,"glAttachShader")&&load(linkProgram,"glLinkProgram")&&load(getProgram,"glGetProgramiv")&&load(programLog,"glGetProgramInfoLog")&&load(useProgram,"glUseProgram")&&load(location,"glGetUniformLocation")&&load(uniform1i,"glUniform1i")&&load(uniform4f,"glUniform4f")&&load(activeTexture,"glActiveTexture");
    if(!ok){message("required OpenGL shader functions unavailable");return false;}
    const char* vs="#version 120\nvarying vec2 uv;void main(){gl_Position=gl_Vertex;uv=gl_MultiTexCoord0.xy;}";
    const char* fs=R"GLSL(#version 120
varying vec2 uv;
uniform sampler2D World,McDepth,Overlay,NativeDepth;
uniform vec4 McInfo,HostInfo;
uniform vec4 WarpX,WarpY,WarpZ,WarpOffset;
vec2 storageUv(vec2 logical){return vec2(logical.x,McInfo.w>.5?logical.y:1.-logical.y);}
float linearDepth(vec2 logical){float d=texture2D(McDepth,storageUv(logical)).r;if(McInfo.z>.5)d=1.-d;return McInfo.x*McInfo.y/max(.00001,McInfo.y-d*(McInfo.y-McInfo.x));}
void main(){
  vec2 at=vec2(uv.x,McInfo.w>.5?1.-uv.y:uv.y);
  vec3 q=vec3(uv.x*2.-1.,uv.y*2.-1.,1.);
  vec3 ray=vec3(dot(WarpX.xyz,q),dot(WarpY.xyz,q),dot(WarpZ.xyz,q));
  vec2 oldUv=.5+.5*vec2(ray.x,-ray.y)/max(.00001,ray.z);float mcZ=0.;
  for(int n=0;n<4;n++){float z=linearDepth(oldUv);mcZ=(z-WarpOffset.z)/max(.00001,ray.z);oldUv=.5+.5*vec2(ray.x*mcZ+WarpOffset.x,-ray.y*mcZ-WarpOffset.y)/max(.00001,z);}
  vec4 world=texture2D(World,storageUv(oldUv)),over=texture2D(Overlay,at);
  if(ray.z<=0.||mcZ<=0.||any(lessThan(oldUv,vec2(0.)))||any(greaterThan(oldUv,vec2(1.))))world=vec4(0.);
  float hostD=texture2D(NativeDepth,uv).r;
  float hostZ=abs(HostInfo.y/(2.*hostD-1.+HostInfo.x))/32.;
  if(HostInfo.z>.5&&hostD<.999999&&mcZ>hostZ+.06)world=vec4(0.);
  gl_FragColor=vec4(over.rgb+world.rgb*(1.-over.a),over.a+world.a*(1.-over.a));
})GLSL";
    auto compile=[&](GLenum type,const char* source){
        GLuint shader=createShader(type);shaderSource(shader,1,&source,nullptr);compileShader(shader);GLint status=0;getShader(shader,0x8B81,&status);
        if(!status){char log[1024]{};shaderLog(shader,sizeof(log)-1,nullptr,log);message(log);deleteShader(shader);return GLuint(0);}return shader;
    };
    GLuint vertex=compile(0x8B31,vs),fragment=compile(0x8B30,fs);if(!vertex||!fragment)return false;
    program=createProgram();attachShader(program,vertex);attachShader(program,fragment);linkProgram(program);deleteShader(vertex);deleteShader(fragment);
    GLint linked=0;getProgram(program,0x8B82,&linked);if(!linked){char log[1024]{};programLog(program,sizeof(log)-1,nullptr,log);message(log);program=0;return false;}
    glGenTextures(4,textures);message("client compositor initialized; real shared-frame input enabled");return true;
}
void parameters(){glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812F);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812F);}
}
void MinecraftifyUpdateCompositePose(const float* at,const float* angles,float fov,float aspect){
    livePose={at[0],at[1],at[2],angles[0],angles[1],angles[2],fov,aspect};havePose=true;
}
void MinecraftifyRememberProjection(){
    float p[16];glGetFloatv(GL_PROJECTION_MATRIX,p);
    projectionValid=std::isfinite(p[10])&&std::isfinite(p[14])&&p[14]<0&&std::abs(p[11]+1)<.001f&&std::abs(p[15])<.001f;
    if(projectionValid){projectionA=p[10];projectionB=p[14];if(p[5]>0)verticalFov=2.f*atanf(1.f/p[5])*57.2957795f;}
}
float MinecraftifyNativeVerticalFov(){return verticalFov;}
void MinecraftifyComposite(){
    HGLRC current=wglGetCurrentContext();if(!current)return;
    if(current!=context){context=current;program=0;for(auto& texture:textures)texture=0;width=height=depthWidth=depthHeight=0;received=0;failed=false;reader.close();}
    bool fresh=reader.next(frame);if(fresh)received=GetTickCount64();
    if(!received||GetTickCount64()-received>500||failed)return;
    if(!program&&!initialize()){failed=true;return;}
    GLint oldProgram=0,oldActive=0,bindings[4]{},oldUnpack=4;glGetIntegerv(0x8B8D,&oldProgram);glGetIntegerv(0x84E0,&oldActive);glGetIntegerv(GL_UNPACK_ALIGNMENT,&oldUnpack);
    glPushAttrib(GL_ALL_ATTRIB_BITS);glPixelStorei(GL_UNPACK_ALIGNMENT,4);
    for(int i=0;i<4;i++){activeTexture(0x84C0+i);glGetIntegerv(GL_TEXTURE_BINDING_2D,&bindings[i]);}
    bool resize=width!=int(frame.width)||height!=int(frame.height);
    if(fresh||resize){
        size_t bytes=size_t(frame.width)*frame.height*4;
        for(int i=0;i<3;i++){
            activeTexture(0x84C0+i);glBindTexture(GL_TEXTURE_2D,textures[i]);parameters();
            GLint format=i==1?0x8818:GL_RGBA8;GLenum external=i==1?GL_LUMINANCE:GL_RGBA,type=i==1?GL_FLOAT:GL_UNSIGNED_BYTE;
            const void* pixels=frame.pixels.data()+bytes*i;
            if(resize)glTexImage2D(GL_TEXTURE_2D,0,format,frame.width,frame.height,0,external,type,pixels);
            else glTexSubImage2D(GL_TEXTURE_2D,0,0,0,frame.width,frame.height,external,type,pixels);
        }width=int(frame.width);height=int(frame.height);
    }
    GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);
    activeTexture(0x84C3);glBindTexture(GL_TEXTURE_2D,textures[3]);parameters();
    if(depthWidth!=viewport[2]||depthHeight!=viewport[3]){
        glTexImage2D(GL_TEXTURE_2D,0,0x81A6,viewport[2],viewport[3],0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,nullptr);depthWidth=viewport[2];depthHeight=viewport[3];
    }
    glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,viewport[0],viewport[1],viewport[2],viewport[3]);
    useProgram(program);
    const char* names[]={"World","McDepth","Overlay","NativeDepth"};
    for(int i=0;i<4;i++){activeTexture(0x84C0+i);glBindTexture(GL_TEXTURE_2D,textures[i]);uniform1i(location(program,names[i]),i);}
    uniform4f(location(program,"McInfo"),frame.near_clip,frame.far_clip,(frame.flags&4)?1.f:0.f,(frame.flags&2)?0.f:1.f);
    uniform4f(location(program,"HostInfo"),projectionA,projectionB,projectionValid?1.f:0.f,0);
    fusion::CameraPose captured={frame.x,frame.y,frame.z,frame.yaw,frame.pitch,frame.roll,frame.fov,float(frame.width)/frame.height};
    auto warp=fusion::reproject(captured,havePose?livePose:captured);
    uniform4f(location(program,"WarpX"),warp.x[0],warp.x[1],warp.x[2],0);uniform4f(location(program,"WarpY"),warp.y[0],warp.y[1],warp.y[2],0);uniform4f(location(program,"WarpZ"),warp.z[0],warp.z[1],warp.z[2],0);uniform4f(location(program,"WarpOffset"),warp.offset[0],warp.offset[1],warp.offset[2],0);
    glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glDisable(GL_ALPHA_TEST);glDisable(GL_CULL_FACE);glDisable(GL_STENCIL_TEST);glDisable(GL_FOG);glDisable(GL_LIGHTING);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(-1,-1);glTexCoord2f(1,0);glVertex2f(1,-1);glTexCoord2f(1,1);glVertex2f(1,1);glTexCoord2f(0,1);glVertex2f(-1,1);glEnd();
    useProgram(oldProgram);
    for(int i=0;i<4;i++){activeTexture(0x84C0+i);glBindTexture(GL_TEXTURE_2D,bindings[i]);}
    activeTexture(oldActive);glPixelStorei(GL_UNPACK_ALIGNMENT,oldUnpack);glPopAttrib();
}

// MIT. Frame transfer/composite approach derived from Rehan's universal-modder.
// This add-on knows nothing about a host executable, mod loader, camera or gameplay.
#include <windows.h>
#include <reshade.hpp>
#include <fusion/frame_reader.hpp>
#include <fusion/pose_channel.hpp>
#include <array>
#include <memory>
#include <mutex>
#include <unordered_map>

using namespace reshade::api;
namespace {
constexpr const char* effect="Minecraftify.fx";
struct Layer { resource texture{};resource_view view{}; };
struct Context {
    fusion::FrameReader reader;
    fusion::Frame frame;
    fusion::PoseReader pose;
    std::array<Layer,3> layers;
    uint32_t width{},height{};
    ULONGLONG received{};
    bool uploaded{};
};
std::mutex lock;
std::unordered_map<effect_runtime*,std::unique_ptr<Context>> contexts;
void release(device* dev,Context& c) {
    for(auto& layer:c.layers) {
        if(layer.view.handle)dev->destroy_resource_view(layer.view);
        if(layer.texture.handle)dev->destroy_resource(layer.texture);
        layer={};
    }
    c.width=c.height=0;c.uploaded=false;
}
void bind(effect_runtime* runtime,Context& c) {
    const char* names[]={"MCWORLD","MCDEPTH","MCOVERLAY"};
    for(int i=0;i<3;++i)runtime->update_texture_bindings(names[i],c.layers[i].view,c.layers[i].view);
}
bool allocate(effect_runtime* rt,Context& c) {
    rt->get_command_queue()->wait_idle();release(rt->get_device(),c);bind(rt,c);
    format formats[]={format::r8g8b8a8_unorm,format::r32_float,format::r8g8b8a8_unorm};
    for(int i=0;i<3;++i) {
        auto& l=c.layers[i];
        if(!rt->get_device()->create_resource(resource_desc(c.frame.width,c.frame.height,1,1,formats[i],1,memory_heap::default_,resource_usage::shader_resource|resource_usage::copy_dest),nullptr,resource_usage::shader_resource,&l.texture)||
            !rt->get_device()->create_resource_view(l.texture,resource_usage::shader_resource,resource_view_desc(formats[i]),&l.view)) {
            release(rt->get_device(),c);bind(rt,c);return false;
        }
    }
    c.width=c.frame.width;c.height=c.frame.height;bind(rt,c);return true;
}
void draw(effect_runtime* rt,command_list*,resource_view,resource_view) {
    std::lock_guard<std::mutex> guard(lock);
    auto& ptr=contexts[rt];if(!ptr)ptr=std::make_unique<Context>();auto& c=*ptr;
    if(c.reader.next(c.frame)) {
        if((c.width==c.frame.width&&c.height==c.frame.height)||allocate(rt,c)) {
            // Conservative complete-image upload. Optimize per graphics API only after a real port's measurements.
            auto* queue=rt->get_command_queue();auto* cmd=queue->get_immediate_command_list();
            if(cmd) {
                resource images[]={c.layers[0].texture,c.layers[1].texture,c.layers[2].texture};
                resource_usage before[]={resource_usage::shader_resource,resource_usage::shader_resource,resource_usage::shader_resource};
                resource_usage after[]={resource_usage::copy_dest,resource_usage::copy_dest,resource_usage::copy_dest};
                bool immediate=rt->get_device()->get_api()==device_api::d3d9;
                cmd->barrier(3,images,before,after);if(!immediate)queue->wait_idle();
                uint32_t bytes=c.width*c.height*4;
                for(int i=0;i<3;++i)rt->get_device()->update_texture_region(subresource_data{c.frame.pixels.data()+size_t(bytes)*i,c.width*4,bytes},images[i],0);
                cmd=queue->get_immediate_command_list();
                if(cmd){cmd->barrier(3,images,after,before);if(!immediate)queue->wait_idle();c.uploaded=true;c.received=GetTickCount64();}
            }
        }
    }
    bool active=c.uploaded&&GetTickCount64()-c.received<500;
    if(auto v=rt->find_uniform_variable(effect,"McActive");v.handle)rt->set_uniform_value_bool(v,active);
    if(active) {
        if(auto v=rt->find_uniform_variable(effect,"McPlanes");v.handle)rt->set_uniform_value_float(v,c.frame.near_clip,c.frame.far_clip,float(c.frame.flags));
        fusion::CameraPose old={c.frame.x,c.frame.y,c.frame.z,c.frame.yaw,c.frame.pitch,c.frame.roll,c.frame.fov,float(c.frame.width)/c.frame.height},now=old;c.pose.latest(now);
        auto warp=fusion::reproject(old,now);
        const char* names[]={"WarpX","WarpY","WarpZ","WarpOffset"};const fusion::Vec3 values[]={warp.x,warp.y,warp.z,warp.offset};
        for(int n=0;n<4;n++)if(auto v=rt->find_uniform_variable(effect,names[n]);v.handle)rt->set_uniform_value_float(v,values[n][0],values[n][1],values[n][2]);
    }
}
void reloaded(effect_runtime* rt) {
    std::lock_guard<std::mutex> guard(lock);auto it=contexts.find(rt);if(it!=contexts.end()&&it->second->uploaded)bind(rt,*it->second);
}
void destroyed(effect_runtime* rt) {
    std::lock_guard<std::mutex> guard(lock);auto it=contexts.find(rt);if(it==contexts.end())return;
    rt->get_command_queue()->wait_idle();release(rt->get_device(),*it->second);bind(rt,*it->second);contexts.erase(it);
}
}
extern "C" __declspec(dllexport) const char* NAME="MinecraftifyAnything Renderer";
extern "C" __declspec(dllexport) const char* DESCRIPTION="Game-neutral Minecraft frame compositor; host adapter required.";
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        if(!reshade::register_addon(module))return FALSE;
        reshade::register_event<reshade::addon_event::reshade_begin_effects>(draw);
        reshade::register_event<reshade::addon_event::reshade_reloaded_effects>(reloaded);
        reshade::register_event<reshade::addon_event::destroy_effect_runtime>(destroyed);
    } else if(reason==DLL_PROCESS_DETACH)reshade::unregister_addon(module);
    return TRUE;
}

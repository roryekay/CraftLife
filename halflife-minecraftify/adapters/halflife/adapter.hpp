#pragma once
#include <string>

namespace minecraftify {
struct Camera {
    // All values already converted into Minecraft units/axes; yaw0=+Z, pitch positive down.
    double x{},y{},z{},feet_x{},feet_y{},feet_z{};
    float yaw{},pitch{},roll{},vertical_fov{70};
    bool first_person{true},control{};
};
class HostAdapter {
public:
    virtual ~HostAdapter()=default;
    // Called on YOUR host update thread. Return false during menus/loading/disconnects.
    virtual bool camera(Camera& out)=0;
    // Received JSON must be parsed/validated, then applied on the correct game thread.
    virtual void guest_event(const std::string& json)=0;
    virtual void release_inputs()=0;
};
class BlankAdapter final:public HostAdapter {
public:
    bool camera(Camera&) override { return false; } // No pretend camera or successful gameplay.
    void guest_event(const std::string&) override {}
    void release_inputs() override {}
};
}

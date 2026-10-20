#pragma once
#include "wire.hpp"
#include <deque>
#include <mutex>
#include <atomic>
namespace mcvalve {
// Subclass only this game's own window; normal Unicode/autorepeat and editing
// keys are forwarded only while Minecraft chat owns focus. No global hook.
class TextInput {
    Link* link_=nullptr;HWND window_=nullptr;WNDPROC original_=nullptr;std::atomic<bool> active_{false};
    std::mutex mutex_;std::deque<Packet> pending_;
    static inline TextInput* owner_=nullptr;
    static LRESULT CALLBACK procedure(HWND window,UINT message,WPARAM w,LPARAM l){
        auto* self=owner_;if(!self||!self->original_)return DefWindowProcW(window,message,w,l);
        if(self->active_&&self->link_&&self->link_->active()&&GetForegroundWindow()==window){
            Packet p;p.kind=32;
            if(message==WM_CHAR&&w>=32&&w<=0x10ffff){p.i[0]=int(w);p.i[2]=1;}
            if(message==WM_KEYDOWN){switch(w){
                case VK_RETURN:p.i[0]=13;break;case VK_ESCAPE:p.i[0]=27;break;case VK_BACK:p.i[0]=8;break;case VK_DELETE:p.i[0]=127;break;case VK_TAB:p.i[0]=9;break;
                case VK_LEFT:p.i[0]=1073741904;break;case VK_RIGHT:p.i[0]=1073741903;break;case VK_UP:p.i[0]=1073741906;break;case VK_DOWN:p.i[0]=1073741905;break;
                case VK_HOME:p.i[0]=1073741898;break;case VK_END:p.i[0]=1073741901;break;
            }}
            if(p.i[0]){p.i[1]=((GetKeyState(VK_SHIFT)&0x8000)?1:0)|((GetKeyState(VK_CONTROL)&0x8000)?64:0);std::lock_guard<std::mutex> lock(self->mutex_);if(self->pending_.size()<512)self->pending_.push_back(p);}
            if(message==WM_CHAR||message==WM_KEYDOWN||message==WM_KEYUP)return 0;
        }
        return CallWindowProcW(self->original_,window,message,w,l);
    }
public:
    ~TextInput(){if(window_&&IsWindow(window_)&&reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window_,GWLP_WNDPROC))==procedure)SetWindowLongPtrW(window_,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(original_));if(owner_==this)owner_=nullptr;}
    void poll(Link& link,bool active){
        link_=&link;
        HWND window=GetForegroundWindow();DWORD pid=0;GetWindowThreadProcessId(window,&pid);
        if(window_&&(!IsWindow(window_)||(pid==GetCurrentProcessId()&&window!=window_))){
            if(IsWindow(window_)&&reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window_,GWLP_WNDPROC))==procedure)SetWindowLongPtrW(window_,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(original_));
            window_=nullptr;original_=nullptr;
        }
        if(!window_){
            if(pid==GetCurrentProcessId()){owner_=this;SetLastError(0);auto original=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(procedure)));if(original){window_=window;original_=original;}}
        }
        std::lock_guard<std::mutex> lock(mutex_);active_=active;
        if(active)for(auto& p:pending_)link.send(p);
        pending_.clear();
    }
};
}

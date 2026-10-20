#pragma once
#include "frame_protocol.hpp"
#include <windows.h>
#include <string>

namespace fusion {
/** One consumer instance per render runtime. Copies a coherent snapshot before GPU upload. */
class FrameReader {
    HANDLE mapping_{},producer_{};
    const uint8_t* view_{};
    uint64_t last_capture_{};
    Frame scratch_;
    ULONGLONG next_open_{};
    std::wstring name_;
public:
    explicit FrameReader(std::wstring name=L"Local\\MinecraftifyAnythingFrame"):name_(std::move(name)){}
    FrameReader(const FrameReader&)=delete;
    FrameReader& operator=(const FrameReader&)=delete;
    ~FrameReader(){close();}
    void close() {
        if(view_)UnmapViewOfFile(view_);if(mapping_)CloseHandle(mapping_);if(producer_)CloseHandle(producer_);
        view_=nullptr;mapping_=producer_=nullptr;last_capture_=0;
    }
    bool next(Frame& frame) {
        if(producer_&&WaitForSingleObject(producer_,0)!=WAIT_TIMEOUT)close();
        if(!view_) {
            if(GetTickCount64()<next_open_)return false;next_open_=GetTickCount64()+500;
            mapping_=OpenFileMappingW(FILE_MAP_READ,FALSE,name_.c_str());if(!mapping_)return false;
            view_=static_cast<const uint8_t*>(MapViewOfFile(mapping_,FILE_MAP_READ,0,0,mapping_size));
            if(!view_||!valid_header(view_)){close();return false;}
            producer_=OpenProcess(SYNCHRONIZE,FALSE,read<DWORD>(view_+44));
            if(!producer_){close();return false;}
        }
        int slot=read<int32_t>(view_+40);if(slot<0||slot>=int(slots))return false;
        const uint8_t* desc=view_+256+128*slot;
        std::array<uint8_t,128> copy;uint64_t seq=read<uint64_t>(desc);MemoryBarrier();
        if(seq&1)return false;std::memcpy(copy.data(),desc,copy.size());
        if(!descriptor(copy.data(),scratch_)||scratch_.sequence!=seq||scratch_.capture<=last_capture_)return false;
        size_t bytes=size_t(scratch_.width)*scratch_.height*12;scratch_.pixels.resize(bytes);
        std::memcpy(scratch_.pixels.data(),view_+header_size+slot_stride*slot,bytes);MemoryBarrier();
        if(seq!=read<uint64_t>(desc))return false;
        last_capture_=scratch_.capture;std::swap(frame,scratch_);return true;
    }
};
}

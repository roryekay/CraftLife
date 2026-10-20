#include <fusion/frame_reader.hpp>
#include <chrono>
#include <iostream>
#include <thread>
int main(int argc,char** argv) {
    int seconds=argc>1?std::stoi(argv[1]):10;
    fusion::FrameReader reader;fusion::Frame frame;unsigned seen=0;
    bool measureAlpha=argc>2&&std::string(argv[2])=="--world-alpha";
    size_t firstAlpha=0,maxAlpha=0;
    auto end=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);
    while(std::chrono::steady_clock::now()<end) {
        if(reader.next(frame)) {
            ++seen;
            if(measureAlpha){size_t alpha=0;for(size_t n=3;n<size_t(frame.width)*frame.height*4;n+=4)if(frame.pixels[n]>16)alpha++;if(seen==1)firstAlpha=alpha;maxAlpha=std::max(maxAlpha,alpha);}
            if(seen==1||seen%60==0)std::cout<<"{\"capture\":"<<frame.capture<<",\"hostFrame\":"<<frame.host_frame<<",\"width\":"<<frame.width<<",\"height\":"<<frame.height<<",\"bytes\":"<<frame.pixels.size()<<"}"<<std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    std::cout<<"{\"frames\":"<<seen;if(measureAlpha)std::cout<<",\"firstWorldAlpha\":"<<firstAlpha<<",\"maxWorldAlpha\":"<<maxAlpha;std::cout<<"}"<<std::endl;return seen?0:1;
}

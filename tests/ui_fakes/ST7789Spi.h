#pragma once
#include "Arduino.h"
#include <array>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
constexpr int GEOMETRY_RAWMODE=0,WHITE=1,BLACK=0,ST77XX_WHITE=0xffff;
class ST7789Spi {
    const std::uint8_t* font_=nullptr;
    int color_=WHITE;
    void pixel(int x,int y){if(x>=0 && x<240 && y>=0 && y<135)pixels[y*240+x]=color_;}
    static unsigned code(const char* text,std::size_t& at,std::size_t size) {
        auto byte=std::uint8_t(text[at++]);if(byte<128)return byte;
        if((byte==0xc2 || byte==0xc3) && at<size && text[at])return ((byte&31)<<6)|(std::uint8_t(text[at++])&63);
        while(at<size && text[at] && (std::uint8_t(text[at])&0xc0)==0x80)++at;
        return '?';
    }
public:
    inline static std::array<std::uint8_t,240*135> pixels{};
    struct DrawCall {int x,y;std::string text;};
    inline static std::vector<DrawCall> draws;
    inline static std::vector<std::string> strings;
    template<class... Args> explicit ST7789Spi(Args...){}
    bool init(){return true;}
    void flipScreenVertically(){}
    void setRGB(int){}
    void setBackground(std::uint16_t){}
    void setFont(const std::uint8_t* font){font_=font;}
    void setColor(int color){color_=color;}
    void clear(){pixels.fill(0);strings.clear();draws.clear();}
    void display(){}
    std::uint16_t getStringWidth(const char* text,std::size_t size=SIZE_MAX,bool=true) {
        if(size==SIZE_MAX)size=std::strlen(text);unsigned width=0;
        for(std::size_t at=0;at<size && text[at];){auto c=code(text,at,size);if(c>=font_[2] && c<font_[2]+font_[3])width+=font_[4+4*(c-font_[2])+3];}
        return width;
    }
    void drawString(int x,int y,const char* text) {
        strings.emplace_back(text);draws.push_back({x,y,text});auto size=std::strlen(text);
        for(std::size_t at=0;at<size;){auto c=code(text,at,size);if(c<font_[2] || c>=font_[2]+font_[3])continue;
            auto entry=font_+4+4*(c-font_[2]);auto width=entry[3];auto stride=(font_[1]+7)/8;
            if(entry[0]!=255 || entry[1]!=255){auto bitmap=font_+4+4*font_[3]+(entry[0]<<8)+entry[1];
                for(unsigned col=0;col<width;++col)for(unsigned row=0;row<font_[1];++row){auto index=col*stride+row/8;if(index<entry[2] && bitmap[index]&(1<<(row%8)))pixel(x+col,y+row);}}
            x+=width;
        }
    }
    void drawHorizontalLine(int x,int y,int width){for(int i=0;i<width;++i)pixel(x+i,y);}
    void fillRect(int x,int y,int width,int height){for(int dy=0;dy<height;++dy)for(int dx=0;dx<width;++dx)pixel(x+dx,y+dy);}
    static void capture(const char* name){std::filesystem::create_directories(".pio/ui-preview");std::ofstream out(std::string(".pio/ui-preview/")+name+".gray",std::ios::binary);for(auto p:pixels)out.put(char(p*255));}
};

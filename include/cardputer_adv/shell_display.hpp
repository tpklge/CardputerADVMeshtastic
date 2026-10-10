#pragma once
#include <ST7789Spi.h>
#if defined(CARDPUTER_ADV_UI_V2) && !defined(CARDPUTER_ADV_UI_NATIVE)
#include "lvgl_surface.hpp"
namespace cardputer_adv {
class ShellDisplay:public LvglSurface {
    SPIClass* spi_;std::uint8_t dc_,cs_;ST7789Spi boot_;
    void push(lv_disp_drv_t* d,const lv_area_t* a,lv_color_t* pixels);
public:
    ShellDisplay(SPIClass* spi,std::uint8_t rst,std::uint8_t dc,std::uint8_t cs,OLEDDISPLAY_GEOMETRY g,int w,int h,int mosi,int miso,int clk):spi_(spi),dc_(dc),cs_(cs),boot_(spi,rst,dc,cs,g,w,h,mosi,miso,clk){}
    bool init();
    void flipScreenVertically(){boot_.flipScreenVertically();}
    void display(){LvglSurface::display(millis());}
};
}
#else
namespace cardputer_adv {using ShellDisplay=ST7789Spi;}
#endif

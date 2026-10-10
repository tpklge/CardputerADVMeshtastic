#pragma once
#ifdef CARDPUTER_ADV_UI_V2
#include <lvgl.h>
#include <array>
#include <cstdint>
#include <cstddef>
namespace cardputer_adv {
using LvglFlush=void(*)(lv_disp_drv_t*,const lv_area_t*,lv_color_t*);
class LvglSurface {
    lv_disp_draw_buf_t draw_{};lv_disp_drv_t driver_{};lv_disp_t* screen_=nullptr;
    std::array<lv_color_t,240*14> pixels_{};
    std::array<lv_obj_t*,32> labels_{};std::array<lv_obj_t*,16> rects_{};
    std::size_t label_count_=0,rect_count_=0;std::uint16_t rgb_=0xffff,ink_=0xffff,background_=0x0841;std::uint32_t last_tick_=0;
public:
    bool begin(LvglFlush flush,void* context,std::uint32_t now);
    void clear();
    void setBackground(std::uint16_t rgb);
    void display(std::uint32_t now);
    void setRGB(std::uint16_t rgb){rgb_=rgb;ink_=rgb;}
    void setColor(int color){ink_=color?rgb_:0;}
    void setFont(const std::uint8_t*){} // Preserves the proven Adv_Plain_12 font.
    std::uint16_t getStringWidth(const char* text,std::size_t size=SIZE_MAX,bool=true);
    void drawString(int x,int y,const char* text);
    void drawHorizontalLine(int x,int y,int width){fillRect(x,y,width,1);}
    void fillRect(int x,int y,int width,int height);
};
}
#endif

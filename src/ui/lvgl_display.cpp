#if defined(CARDPUTER_ADV_UI_V2) && !defined(CARDPUTER_ADV_UI_NATIVE)
#include "cardputer_adv/shell_display.hpp"
namespace cardputer_adv {
bool ShellDisplay::init(){if(!boot_.init())return false;return begin([](lv_disp_drv_t* d,const lv_area_t* a,lv_color_t* p){static_cast<ShellDisplay*>(d->user_data)->push(d,a,p);},this,millis());}
void ShellDisplay::push(lv_disp_drv_t* d,const lv_area_t* a,lv_color_t* pixels){spi_->beginTransaction(SPISettings(40000000,MSBFIRST,SPI_MODE0));digitalWrite(cs_,LOW);auto command=[&](std::uint8_t c){digitalWrite(dc_,LOW);spi_->transfer(c);digitalWrite(dc_,HIGH);};auto bounds=[&](unsigned start,unsigned end){spi_->transfer(start>>8);spi_->transfer(start);spi_->transfer(end>>8);spi_->transfer(end);};command(ST77XX_CASET);bounds(a->x1+40,a->x2+40);command(ST77XX_RASET);bounds(a->y1+52,a->y2+52);command(ST77XX_RAMWR);spi_->writeBytes(reinterpret_cast<std::uint8_t*>(pixels),(a->x2-a->x1+1)*(a->y2-a->y1+1)*2);digitalWrite(cs_,HIGH);spi_->endTransaction();lv_disp_flush_ready(d);}
}
#endif

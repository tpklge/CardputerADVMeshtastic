#ifdef CARDPUTER_ADV_M8
#include "cardputer_adv/notifications.hpp"
#include "configuration.h"
#include <Wire.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <cmath>
#include <algorithm>

namespace cardputer_adv {
namespace {
constexpr i2s_port_t port=I2S_NUM_1;
QueueHandle_t requests=nullptr;
bool ready=false;
bool codec_write(std::uint8_t reg,std::uint8_t value){
    Wire.beginTransmission(0x18);Wire.write(reg);Wire.write(value);
    return Wire.endTransmission()==0;
}
void audio_worker(void*){
    // Stereo slots with identical samples: 2 kHz, 320 ms, strong output
    // with a 5 ms envelope to avoid clicks. DMA waits stay off the radio/UI loop.
    std::int16_t samples[128*2];std::uint8_t request;
    while(true){
        if(xQueueReceive(requests,&request,portMAX_DELAY)!=pdTRUE)continue;
        i2s_zero_dma_buffer(port);i2s_start(port);
        for(unsigned offset=0;offset<5120;offset+=128){
            for(unsigned i=0;i<128;++i){auto n=offset+i;
                auto envelope=std::min(1.0f,std::min(n/80.0f,(5119-n)/80.0f));
                auto value=std::int16_t(28000*envelope*std::sin(n*6.28318530718f/8));
                samples[2*i]=samples[2*i+1]=value;
            }
            std::size_t written=0;
            if(i2s_write(port,samples,sizeof(samples),&written,pdMS_TO_TICKS(40))!=ESP_OK || written!=sizeof(samples))break;
        }
        // Drain the bounded DMA tail before stopping the clock.
        std::int16_t silence[128*2]{};std::size_t written=0;
        for(unsigned i=0;i<5;++i)i2s_write(port,silence,sizeof(silence),&written,pdMS_TO_TICKS(40));
        i2s_stop(port);
    }
}
}
bool notification_audio_begin(){
    if(ready)return true;
    i2s_config_t config{};config.mode=i2s_mode_t(I2S_MODE_MASTER|I2S_MODE_TX);
    config.sample_rate=16000;config.bits_per_sample=I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format=I2S_CHANNEL_FMT_RIGHT_LEFT;
    config.communication_format=I2S_COMM_FORMAT_STAND_I2S;
    config.intr_alloc_flags=ESP_INTR_FLAG_LEVEL1;config.dma_buf_count=4;
    config.dma_buf_len=128;config.tx_desc_auto_clear=true;
    if(i2s_driver_install(port,&config,0,nullptr)!=ESP_OK)return false;
    i2s_pin_config_t pins{};pins.mck_io_num=I2S_PIN_NO_CHANGE;
    pins.bck_io_num=DAC_I2S_BCK;pins.ws_io_num=DAC_I2S_WS;
    pins.data_out_num=DAC_I2S_DOUT;pins.data_in_num=I2S_PIN_NO_CHANGE;
    if(i2s_set_pin(port,&pins)!=ESP_OK){i2s_driver_uninstall(port);return false;}
    // ES8311 register settings verified against M5Unified 0.2.18's
    // Cardputer ADV enable callback; codec derives its clock from BCLK.
    constexpr std::uint8_t regs[][2]={{0x00,0x80},{0x01,0xb5},{0x02,0x18},
        {0x0d,0x01},{0x12,0x00},{0x13,0x10},{0x32,0xbf},{0x37,0x08}};
    for(auto& reg:regs)if(!codec_write(reg[0],reg[1])){i2s_driver_uninstall(port);return false;}
    i2s_zero_dma_buffer(port);i2s_stop(port);
    requests=xQueueCreate(1,sizeof(std::uint8_t));
    if(!requests){i2s_driver_uninstall(port);return false;}
    if(xTaskCreate(audio_worker,"adv-beep",3072,nullptr,1,nullptr)!=pdPASS){
        vQueueDelete(requests);requests=nullptr;i2s_driver_uninstall(port);return false;
    }
    ready=true;return true;
}
void notification_audio_beep(){if(ready){std::uint8_t request=1;xQueueOverwrite(requests,&request);}}
}
#endif

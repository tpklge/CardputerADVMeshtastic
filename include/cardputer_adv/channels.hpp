#pragma once
#ifdef CARDPUTER_ADV_M6
#include "channel_url.hpp"
#include <cstdint>
#include <cstddef>
#include "mesh_backend.hpp"
#include "keyboard_decoder.hpp"
#if defined(CARDPUTER_ADV_UI_V2) && !defined(CARDPUTER_ADV_UI_NATIVE)
namespace cardputer_adv {class ShellDisplay;}
#else
class ST7789Spi;
namespace cardputer_adv {using ShellDisplay=ST7789Spi;}
#endif
namespace cardputer_adv {
struct ChannelPlan {meshtastic_Channel slots[8]{};meshtastic_Config_LoRaConfig lora{};std::uint32_t revision=0;bool apply_lora=false;};
bool channel_plan_import(ChannelPlan& plan,const meshtastic_ChannelSet& set,bool replace,bool radio);
bool channel_plan_valid(const ChannelPlan& plan);
bool channel_key_parse(const char* input,meshtastic_ChannelSettings_psk_t& key);
void channel_key_text(const meshtastic_ChannelSettings_psk_t& key,char* out);
bool mesh_channels_snapshot(ChannelPlan& plan);
bool mesh_channels_apply(const ChannelPlan& plan,std::uint32_t token);
bool mesh_channels_export(int index,char* url,std::size_t capacity);
std::uint32_t mesh_channels_revision();
void m6_channels_key(const KeyEvent& event,std::uint8_t selected=255);
void m6_channels_draw(ShellDisplay& display,int bottom,std::uint8_t selected);
void m6_channels_tick();
bool m6_channels_root();
void m6_channels_event(const MeshEvent& event);
}
#endif

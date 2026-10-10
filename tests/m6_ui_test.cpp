#include "cardputer_adv/channels.hpp"
#include "cardputer_adv/app_storage.hpp"
#include "cardputer_adv/font12.hpp"
#include <ST7789Spi.h>
#include <cassert>
#include <cstring>
using namespace cardputer_adv;
ChannelPlan current,applied;unsigned calls=0;std::uint32_t token=0;char draft[2048]{};std::uint16_t cursor=0;bool exported=false;unsigned selected=1;
ST7789Spi display;
namespace cardputer_adv {
StorageStatus app_storage_status(){StorageStatus s;s.ready=true;return s;}
MeshSnapshot mesh_snapshot(){MeshSnapshot s;s.ready=true;return s;}
bool mesh_channels_snapshot(ChannelPlan& p){p=current;return true;}
bool mesh_channels_apply(const ChannelPlan& p,std::uint32_t t){applied=p;token=t;++calls;return true;}
std::uint32_t mesh_channels_revision(){return current.revision;}
std::size_t mesh_channels_page(std::size_t,MeshChannel* out,std::size_t cap){auto n=std::min<std::size_t>(8,cap);for(unsigned i=0;i<n;++i){out[i]={};out[i].role=current.slots[i].role;std::snprintf(out[i].name,sizeof(out[i].name),"%s",current.slots[i].settings.name);}return n;}
bool app_storage_url(const char* text,std::uint16_t c,bool export_file){std::strcpy(draft,text);cursor=c;exported|=export_file;return true;}
bool app_storage_url_load(char* out,std::size_t,std::uint16_t& c){std::strcpy(out,draft);c=cursor;return true;}
bool mesh_channels_export(int,char* out,std::size_t cap){meshtastic_ChannelSet s{};s.settings_count=1;s.settings[0]=current.slots[1].settings;return channel_url_encode(s,false,out,cap);}
}
void draw(){display.clear();display.setFont(Adv_Plain_12);display.setColor(WHITE);m6_channels_draw(display,117,selected);}
void key(KeyAction action,char c=0,bool repeat=false){m6_channels_key({action,c,repeat});draw();}
bool has(const char* v){for(auto& s:ST7789Spi::strings)if(s.find(v)!=std::string::npos)return true;return false;}
void done(){current=applied;++current.revision;MeshEvent e;e.kind=MeshEventKind::Configured;e.token=token;m6_channels_event(e);m6_channels_tick();draw();}
int main(){current.revision=1;for(unsigned i=0;i<8;++i)current.slots[i].index=i;current.slots[0].role=meshtastic_Channel_Role_PRIMARY;current.slots[0].has_settings=true;current.slots[0].settings.psk.size=1;current.slots[0].settings.psk.bytes[0]=1;m6_channels_tick();draw();key(KeyAction::Enter);key(KeyAction::Enter);assert(has("Nome do canal"));for(char c:std::string("BR2"))key(KeyAction::Text,c);key(KeyAction::Enter);assert(!calls && has("Chave"));key(KeyAction::Enter);assert(!calls && has("PSK: 32"));ST7789Spi::capture("m6-create-review");key(KeyAction::Enter);assert(calls==1 && applied.slots[1].role==2 && !std::strcmp(applied.slots[1].settings.name,"BR2"));done();
 meshtastic_ChannelSet s{};s.settings_count=1;std::strcpy(s.settings[0].name,"Amigos");s.settings[0].psk.size=16;std::memset(s.settings[0].psk.bytes,12,16);assert(channel_url_encode(s,true,draft,sizeof(draft)));cursor=std::strlen(draft);
 key(KeyAction::Enter);for(unsigned i=0;i<3;++i)key(KeyAction::Down);key(KeyAction::Enter);assert(has("Link Meshtastic"));key(KeyAction::Enter);assert(has("ADICIONAR") && has("slot 0"));ST7789Spi::capture("m6-import-mode");key(KeyAction::Enter);assert(calls==1);key(KeyAction::Down);key(KeyAction::Down);assert(has("Amigos"));key(KeyAction::Enter);assert(calls==1 && has("Confirmar"));key(KeyAction::Enter);assert(calls==2 && !std::strcmp(applied.slots[2].settings.name,"Amigos"));done();
 key(KeyAction::Enter);for(unsigned i=0;i<4;++i)key(KeyAction::Down);key(KeyAction::Enter);assert(has("Exportar link com chaves"));key(KeyAction::Enter,0,true);assert(!exported);key(KeyAction::Enter);assert(exported && has("Link Meshtastic"));ST7789Spi::capture("m6-export-url");
}

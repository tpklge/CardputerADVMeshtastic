#if defined(CARDPUTER_ADV_M2) && !defined(CARDPUTER_ADV_M4)
#include "cardputer_adv/m2_console.hpp"
#include "cardputer_adv/mesh_backend.hpp"
#include "cardputer_adv/core_storage.hpp"
#include "configuration.h"
#include "Throttle.h"
#include <ST7789Spi.h>
#include <OLEDDisplayFonts.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#ifdef CARDPUTER_ADV_M3
#include "cardputer_adv/app_storage.hpp"
#include "cardputer_adv/font12.hpp"
#endif
namespace cardputer_adv {
namespace {
SPIClass display_spi(HSPI);
ST7789Spi display(&display_spi,ST7789_RESET,ST7789_RS,ST7789_NSS,GEOMETRY_RAWMODE,TFT_WIDTH,TFT_HEIGHT,ST7789_SDA,ST7789_MISO,ST7789_SCK);
bool ready=false;
char input[kMeshTextBytes+1]{},message[kMeshTextBytes+1]="Iniciando";
std::size_t cursor=0,node_offset=0;
unsigned view=0;
#ifdef CARDPUTER_ADV_M3
constexpr unsigned views=4;
bool restored=false;
std::uint64_t history_before=0;
std::array<StoredRecord,4> history_page{};
std::size_t history_count=0;
unsigned history_selection=0,history_text_offset=0;
char pending_text[kMeshTextBytes+1]{};
std::uint32_t pending_token=0;
constexpr int row_height=16;
#else
constexpr unsigned views=3;
constexpr int row_height=13;
#endif
std::uint32_t frame=0,token=0,last_packet=0;
const char* regions[]={"UNSET","US","EU_433","EU_868","CN","JP","ANZ","KR","TW","RU","IN","NZ_865","TH","LORA_24","UA_433","UA_868","MY_433","MY_919","SG_923","PH_433","PH_868","PH_915","ANZ_433","KZ_433","KZ_863","NP_865","BR_902","ITU1_2M","ITU2_2M","EU_866","EU_874","EU_917","EU_N_868","ITU3_2M"};
const char* presets[]={"LONG_FAST","LONG_SLOW","VERY_LONG_SLOW","MEDIUM_SLOW","MEDIUM_FAST","SHORT_SLOW","SHORT_FAST","LONG_MODERATE","SHORT_TURBO","LONG_TURBO","LITE_FAST","LITE_SLOW","NARROW_FAST","NARROW_SLOW"};
void line(int y,const char* text){display.drawString(2,y,text);}
void execute(bool send) {
    if(!input[0])return;
    MeshCommand c;c.token=++token;
    auto s=mesh_snapshot();c.region=s.region;c.preset=s.preset;c.frequency_slot=s.frequency_slot;c.hop_limit=s.hop_limit;
    bool parsed=true;
#ifdef CARDPUTER_ADV_M3
    if(std::strcmp(input,"/sd")==0) {
        auto status=app_storage_status();char diagnostic[180];
        std::snprintf(diagnostic,sizeof(diagnostic),"SD %s: %llu registros; fila %lu; CRC %lu; retirados %llu",status.fault?"ERRO":status.ready?"OK":"--",static_cast<unsigned long long>(status.records),static_cast<unsigned long>(status.dropped),static_cast<unsigned long>(status.corrupt),static_cast<unsigned long long>(status.retired));
        input[0]=0;cursor=0;m2_console_message(diagnostic);return;
    }
    if(std::strncmp(input,"/utc ",5)==0) {
        char* end=nullptr;auto minutes=std::strtol(input+5,&end,10);
        if(end==input+5 || *end || minutes<-720 || minutes>840 || !app_storage_status().ready || !app_storage_offset(minutes)) {m2_console_message("/utc: -720 a 840 minutos; exige SD");return;}
        input[0]=0;cursor=0;m2_console_message("Fuso na fila de gravacao");return;
    }
#endif
    if(std::strncmp(input,"/region ",8)==0) {
        c.kind=MeshCommandKind::SetRadio;parsed=false;
        for(unsigned i=0;i<sizeof(regions)/sizeof(*regions);++i)if(std::strcmp(input+8,regions[i])==0){c.region=i;parsed=true;break;}
    } else if(std::strncmp(input,"/preset ",8)==0) {
        c.kind=MeshCommandKind::SetRadio;parsed=false;
        for(unsigned i=0;i<sizeof(presets)/sizeof(*presets);++i)if(std::strcmp(input+8,presets[i])==0){c.preset=i;parsed=true;break;}
    } else if(std::strncmp(input,"/slot ",6)==0 || std::strncmp(input,"/hops ",6)==0) {
        c.kind=MeshCommandKind::SetRadio;char* end=nullptr;auto value=std::strtol(input+6,&end,10);
        parsed=end!=input+6 && !*end && value>=0 && value<=65535;
        if(input[1]=='s')c.frequency_slot=value;else {parsed=parsed&&value<=7;c.hop_limit=value;}
    } else if(std::strncmp(input,"/name ",6)==0) {
        c.kind=MeshCommandKind::SetOwner;auto space=std::strchr(input+6,' ');parsed=space&&space>input+6&&space-input-6<=4&&std::strlen(space+1)<sizeof(c.long_name);
        if(parsed){std::memcpy(c.short_name,input+6,space-input-6);std::strcpy(c.long_name,space+1);}
    } else if(input[0]=='/') {parsed=false;}
    else if(!send) {m2_console_message("Fn+Enter envia texto; Enter: comando");return;}
    else {std::strcpy(c.text,input);}
    if(!parsed||!mesh_validate_command(c)){m2_console_message("Comando invalido. Tab: ajuda");return;}
    if(!s.ready||s.storage_fault){m2_console_message("Nucleo/SD indisponivel");return;}
#ifdef CARDPUTER_ADV_M3
    if(c.kind==MeshCommandKind::SendText && pending_token){m2_console_message("Envio anterior em gravacao");return;}
#endif
    if(!mesh_submit(c)){m2_console_message("Fila cheia: tente novamente");return;}
#ifdef CARDPUTER_ADV_M3
    if(c.kind==MeshCommandKind::SendText){std::strcpy(pending_text,input);pending_token=c.token;}
#endif
    input[0]=0;cursor=0;m2_console_message("Comando na fila");
}
}
void m2_console_message(const char* text){std::snprintf(message,sizeof(message),"%s",text);}
void m2_console_begin(){frame=millis()-200;pinMode(VTFT_LEDA,OUTPUT);digitalWrite(VTFT_LEDA,HIGH);ready=display.init();if(ready){display.flipScreenVertically();display.setRGB(ST77XX_WHITE);
#ifdef CARDPUTER_ADV_M3
    display.setFont(Adv_Plain_12);
#else
    display.setFont(ArialMT_Plain_10);
#endif
}}
void m2_console_key(const KeyEvent& e) {
    auto n=std::strlen(input);
    auto old_view=view;
    switch(e.action) {
    case KeyAction::Text: if(n<kMeshTextBytes){std::memmove(input+cursor+1,input+cursor,n-cursor+1);input[cursor++]=e.character;}else m2_console_message("Limite M2: 200 bytes");break;
    case KeyAction::Backspace: if(cursor){std::memmove(input+cursor-1,input+cursor,n-cursor+1);--cursor;}break;
    case KeyAction::Left:
#ifdef CARDPUTER_ADV_M3
        if(view==3) {if(history_text_offset>=2)history_text_offset-=2;break;}
#endif
        if(cursor)--cursor;break;
    case KeyAction::Right:
#ifdef CARDPUTER_ADV_M3
        if(view==3) {history_text_offset=std::min<unsigned>(history_text_offset+2,100);break;}
#endif
        if(cursor<n)++cursor;break;
    case KeyAction::Escape: input[0]=0;cursor=0;break;
    case KeyAction::Tab:view=(view+1)%views;break;
    case KeyAction::BackTab:view=(view+views-1)%views;break;
    case KeyAction::Up:
#ifdef CARDPUTER_ADV_M3
        if(view==3) {if(history_selection) {--history_selection;history_text_offset=0;} else if(history_count) {history_before=history_page[history_count-1].sequence;app_storage_page(history_before);history_selection=0;history_text_offset=0;}}
        else
#endif
        if(node_offset>=4)node_offset-=4;break;
    case KeyAction::Down:
#ifdef CARDPUTER_ADV_M3
        if(view==3) {if(history_selection+1<history_count) {++history_selection;history_text_offset=0;} else {history_before=0;app_storage_page();history_selection=0;history_text_offset=0;}}
        else
#endif
        if(node_offset+4<mesh_snapshot().nodes)node_offset+=4;break;
    case KeyAction::Enter:if(!e.repeat)execute(false);break;
    case KeyAction::Send:if(!e.repeat)execute(true);break;
    default:break;
    }
#ifdef CARDPUTER_ADV_M3
    if(view==3 && old_view!=view) {history_before=0;history_selection=0;history_text_offset=0;app_storage_page();}
    if(e.action!=KeyAction::None)app_storage_ui(pending_text[0]?pending_text:input,pending_text[0]?std::strlen(pending_text):cursor,view,mesh_conversation(0),old_view!=view || e.action==KeyAction::Send || e.action==KeyAction::Enter);
#else
    (void)old_view;
#endif
}
void m2_console_tick() {
#ifdef CARDPUTER_ADV_M3
    if(!restored && app_storage_status().ready) {
        StoredRecord saved;
        if(app_storage_load_ui(saved)) {
            if(!saved.conversation || saved.conversation==mesh_conversation(0)) {std::strcpy(input,saved.event.text);cursor=saved.cursor;}
            else m2_console_message("Rascunho de outro canal preservado no SD");
            view=saved.view;if(view==3)app_storage_page();
        }
        restored=true;
    }
#endif
    MeshEvent e;
    while(mesh_poll_event(e)) {
#ifdef CARDPUTER_ADV_M3
        if(e.token && e.token==pending_token) {
            if(e.kind==MeshEventKind::Queued) {pending_text[0]=0;pending_token=0;app_storage_ui(input,cursor,view,mesh_conversation(0),true);}
            else if(e.kind==MeshEventKind::Error) {if(!input[0]) {std::strcpy(input,pending_text);cursor=std::strlen(input);}pending_text[0]=0;pending_token=0;app_storage_ui(input,cursor,view,mesh_conversation(0),true);}
        }
#endif
        char text[kMeshTextBytes+1];
        if(e.kind==MeshEventKind::Queued){last_packet=e.packet_id;std::snprintf(text,sizeof(text),"Fila local #%08lx",static_cast<unsigned long>(e.packet_id));m2_console_message(text);}
        else if(e.kind==MeshEventKind::Transmitted&&e.packet_id==last_packet){std::snprintf(text,sizeof(text),"TX radio #%08lx (sem entrega confirmada)",static_cast<unsigned long>(e.packet_id));m2_console_message(text);}
        else if(e.kind==MeshEventKind::RoutingFeedback&&e.packet_id==last_packet){std::snprintf(text,sizeof(text),"Feedback #%08lx: %ld",static_cast<unsigned long>(e.packet_id),static_cast<long>(e.result));m2_console_message(text);}
        else if(e.kind==MeshEventKind::Text){std::snprintf(text,sizeof(text),"RX !%08lx: %.175s",static_cast<unsigned long>(e.node),e.text);m2_console_message(text);}
        else if(e.kind==MeshEventKind::Configured)m2_console_message("Configuracao salva/aplicada");
        else if(e.kind==MeshEventKind::Error)m2_console_message(e.text);
    }
    if(!ready||Throttle::isWithinTimespanMs(frame,200))return;frame=millis();
    auto s=mesh_snapshot();display.clear();char text[100];
    std::snprintf(text,sizeof(text),"ADV | !%08lx | %s",static_cast<unsigned long>(s.node),s.radio_ready?"RF OK":"RF --");line(0,text);
    if(view==0) {
        std::snprintf(text,sizeof(text),"%s / %s",regions[s.region<34?s.region:0],presets[s.preset<14?s.preset:0]);line(row_height,text);
        std::snprintf(text,sizeof(text),"%.12s s%u %.2fV",s.channel_name,s.frequency_slot,s.battery_mv/1000.f);line(row_height*2,text);
        std::snprintf(text,sizeof(text),"TX %lu RX %lu erro %lu",static_cast<unsigned long>(s.tx),static_cast<unsigned long>(s.rx),static_cast<unsigned long>(s.rx_errors));
#ifdef CARDPUTER_ADV_M3
        auto st=app_storage_status();std::snprintf(text,sizeof(text),"TX %lu RX %lu SD %s",static_cast<unsigned long>(s.tx),static_cast<unsigned long>(s.rx),s.storage_fault||st.fault?"ERRO":st.dropped?"FILA!":st.recovered?"REC":st.ready?"OK":"--");
#endif
        line(row_height*3,text);
        if(s.gps_time)std::snprintf(text,sizeof(text),"UTC %02lu:%02lu:%02lu fix %s sat %u",static_cast<unsigned long>((s.utc/3600)%24),static_cast<unsigned long>((s.utc/60)%60),static_cast<unsigned long>(s.utc%60),s.gps_fix?"OK":"--",s.satellites);
        else std::snprintf(text,sizeof(text),"GPS UTC -- fix %s sat %u",s.gps_fix?"OK":"--",s.satellites);line(row_height*4,text);
#ifndef CARDPUTER_ADV_M3
        std::snprintf(text,sizeof(text),"SD %s heap %lu drop %lu",s.storage_fault?"ERRO":s.storage_ready?"OK":"--",static_cast<unsigned long>(s.heap_min),static_cast<unsigned long>(s.event_drops));line(row_height*5,text);
#endif
    } else if(view==1) {
        MeshNode nodes[4];auto n=mesh_nodes_page(node_offset,nodes,4);
        for(std::size_t i=0;i<n;++i){auto& d=nodes[i];std::snprintf(text,sizeof(text),"!%08lx %-4s %s",static_cast<unsigned long>(d.number),d.short_name,d.number==s.node?"local":d.heard_this_boot?"RX boot":d.heard_known?"cache":"RX ?");line(row_height+row_height*i,text);}
        #ifndef CARDPUTER_ADV_M3
        line(row_height*5,"Fn+setas: pagina de nos");
#endif
    }
#ifdef CARDPUTER_ADV_M3
    else if(view==3) {
        auto st=app_storage_status();
        std::snprintf(text,sizeof(text),"Historico SD: %s",st.busy?"lendo...":st.fault?"ERRO":"OK");line(16,text);
        if(!st.busy)history_count=app_storage_page_read(history_page.data(),history_page.size());
        if(history_count) {
            history_selection=std::min<unsigned>(history_selection,history_count-1);
            auto& record=history_page[history_selection];
            std::snprintf(text,sizeof(text),"%s !%08lx ch%u",record.event.kind==MeshEventKind::Text?"RX":"TX",static_cast<unsigned long>(record.event.node),record.event.channel);line(32,text);
            if(record.utc_valid)std::snprintf(text,sizeof(text),"UTC %02lu:%02lu | %u/%u",static_cast<unsigned long>((record.utc/3600)%24),static_cast<unsigned long>((record.utc/60)%60),history_selection+1,unsigned(history_count));
            else std::snprintf(text,sizeof(text),"Hora desconhecida | %u/%u",history_selection+1,unsigned(history_count));line(48,text);
            std::snprintf(message,sizeof(message),"%s",record.event.text);
        } else if(!st.busy)m2_console_message("Sem mensagens nesta pagina");
                const char* state="RX | Fn+esq/dir: texto";
        if(history_count) {auto& record=history_page[history_selection];if(record.event.kind==MeshEventKind::Queued)state=record.delivery==MeshEventKind::Transmitted?"TX radio":record.delivery==MeshEventKind::RoutingFeedback?(record.event.result?"NAK":"ACK"):record.delivery==MeshEventKind::Error?"Falhou":"Fila / resultado desconhecido";}
        line(64,state);
    }
#endif
    else {
        line(row_height,"/region BR_902");line(row_height*2,"/preset LONG_FAST; /slot 0");line(row_height*3,"/hops 3; /name ADV Nome");line(row_height*4,"Fn+Enter: TX; /utc -240");
#ifndef CARDPUTER_ADV_M3
        line(row_height*5,"Tab: abas; Fn+Esc: limpar");
#endif
    }
    // Wrap at pixel boundaries without splitting UTF-8 codepoints.
    std::size_t offset=0;
    const auto length=std::strlen(message);
#ifdef CARDPUTER_ADV_M3
    const unsigned message_rows=2;
    const unsigned message_top=80;
#else
    const unsigned message_rows=3,message_top=78;
#endif
    const unsigned skip=
#ifdef CARDPUTER_ADV_M3
        view==3?history_text_offset:0;
#else
        0;
#endif
    for(unsigned row=0;row<message_rows+skip;++row) {
        char part[81]{};
        std::size_t count=0;
        while(offset+count<length) {
            auto byte=static_cast<unsigned char>(message[offset+count]);
            auto bytes=byte<0x80?1u:byte<0xe0?2u:byte<0xf0?3u:4u;
            if(count+bytes>=sizeof(part) || offset+count+bytes>length ||
               display.getStringWidth(message+offset,count+bytes,true)>236) break;
            count+=bytes;
        }
        std::memcpy(part,message+offset,count);
        if(row>=skip)line(message_top+row_height*(row-skip),part);
        offset+=count;
    }
    std::size_t start=0;
    while(start<cursor && display.getStringWidth(input+start,cursor-start)>218) ++start;
    std::size_t visible=0;
    while(input[start+visible] && display.getStringWidth(input+start,visible+1)<=224) ++visible;
    char edit[kMeshTextBytes+3]{};
    edit[0]='>'; edit[1]=' ';
    std::memcpy(edit+2,input+start,visible);
    line(119,edit);
    const auto cursor_x=2+display.getStringWidth("> ",2)+display.getStringWidth(input+start,cursor-start);
    display.drawHorizontalLine(cursor_x,133,4);
    display.display();
}
}
#endif

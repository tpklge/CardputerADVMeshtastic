#ifdef CARDPUTER_ADV_M4
#include "cardputer_adv/m2_console.hpp"
#include "cardputer_adv/ui_shell.hpp"
#include "cardputer_adv/app_storage.hpp"
#include "cardputer_adv/font12.hpp"
#include "configuration.h"
#include "cardputer_adv/shell_display.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "cardputer_adv/theme.hpp"
#ifdef CARDPUTER_ADV_M8
#include "cardputer_adv/core_storage.hpp"
#include "cardputer_adv/maintenance.hpp"
#include <esp_system.h>
#endif
#ifdef CARDPUTER_ADV_M7_NODES
#include "cardputer_adv/node_tools.hpp"
#include <ctime>
#endif
#ifdef CARDPUTER_ADV_M6
#include "cardputer_adv/channels.hpp"
std::uint32_t channels_seen_revision=0;
#endif

namespace cardputer_adv {
namespace {
SPIClass display_spi(HSPI);
ShellDisplay display(&display_spi,ST7789_RESET,ST7789_RS,ST7789_NSS,GEOMETRY_RAWMODE,TFT_WIDTH,TFT_HEIGHT,ST7789_SDA,ST7789_MISO,ST7789_SCK);
ShellMenu menu;
UiNavigation nav;
char draft[kMeshTextBytes+1]{},field[64]{},notice[kMeshTextBytes+1]="Iniciando";
TextEditor editor(draft,sizeof(draft)),form(field,sizeof(field));
char pending_text[kMeshTextBytes+1]{};
std::uint32_t pending_tx=0,pending_config=0,token=0,frame=0,last_packet=0;
bool display_ready=false,restored=false,page_pending=false,redraw=true;
std::uint64_t history_records=0,history_requested_records=0;
std::array<StoredRecord,4> history{};
std::size_t history_count=0;
#ifdef CARDPUTER_ADV_UI_V2
bool history_follow=true,history_newer=false;
int history_direction=0;std::uint64_t history_anchor=0,history_previous_before=0;
#endif
#ifdef CARDPUTER_ADV_M5
char history_names[4][16]{};
unsigned detail_page=0;
#endif
StoredRecord detail;
MeshNode node_detail;
#ifdef CARDPUTER_ADV_M7
bool node_notice=false;
#ifdef CARDPUTER_ADV_M7_NODES
char node_search_text[25]{};TextEditor node_search_editor(node_search_text,sizeof(node_search_text));bool node_search_open=false;
std::size_t node_total=0;std::uint32_t favorite_token=0;NodeMetricsStamp node_metrics;
#endif
#endif
std::uint8_t new_rx=0;
enum class ChatPage {List,Compose,Detail,Conversations};
ChatPage chat_page=ChatPage::List;
enum class SettingsPage {List,Edit,Gps,Storage,Diagnostic,Help,ClearConfirm,Theme,Tools,BackupConfirm,RestoreList,RestoreConfirm};
SettingsPage settings_page=SettingsPage::List;
#ifdef CARDPUTER_ADV_M8
unsigned theme_preview=0,tools_selection=0,restore_selection=0;
unsigned restore_slots[64]{};std::size_t restore_count=0;
bool trace_open=false,trace_export_notice=false;unsigned trace_row=0;std::uint32_t trace_token=0;
#endif
const Theme& palette(){
#ifdef CARDPUTER_ADV_M8
    return theme(settings_page==SettingsPage::Theme?theme_preview:nav.theme);
#else
    return theme(0);
#endif
}
bool node_open=false,channel_open=false;
unsigned node_detail_page=0,help_page=0,gps_page=0;
#ifdef CARDPUTER_ADV_M5
MeshChannel chat_channels[8]{};
std::size_t chat_channel_count=0;
std::uint8_t conversation_selection=0;
bool switching_chat=false;
std::uint64_t chat_identity=0;
std::uint64_t active_conversation(){return chat_identity;}
#endif
const char* tab_names[]={"CHAT","NÓS","CANAIS","AJUSTES"};
const char* setting_names[]={"Nome curto","Nome longo","Regiao","Preset","Hops","Slot","Fuso UTC","GPS / hora","Cartao SD","Diagnostico","Teclas / ajuda","Limpar rascunho"
#ifdef CARDPUTER_ADV_M8
,"Tema de cores","Backup / exportar"
#endif
};
constexpr unsigned kSettingCount=sizeof(setting_names)/sizeof(setting_names[0]);
const char* region_names[]={"UNSET","US","EU_433","EU_868","CN","JP","ANZ","KR","TW","RU","IN","NZ_865","TH","LORA_24","UA_433","UA_868","MY_433","MY_919","SG_923","PH_433","PH_868","PH_915","ANZ_433","KZ_433","KZ_863","NP_865","BR_902","ITU1_2M","ITU2_2M","EU_866","EU_874","EU_917","EU_N_868","ITU3_2M"};
const char* preset_names[]={"LONG_FAST","LONG_SLOW","VERY_LONG_SLOW","MEDIUM_SLOW","MEDIUM_FAST","SHORT_SLOW","SHORT_FAST","LONG_MODERATE","SHORT_TURBO","LONG_TURBO","LITE_FAST","LITE_SLOW","NARROW_FAST","NARROW_SLOW"};
const char* region_name(unsigned n){return region_names[n<34?n:0];}
const char* preset_name(unsigned n){return preset_names[n<14?n:0];}
void save(bool force=false) {
    nav.shell=true;
#ifdef CARDPUTER_ADV_M5
    if(switching_chat)return;
    nav.conversations=true;app_storage_ui(editor.text(),editor.cursor(),unsigned(menu.tab()),active_conversation(),force,&nav);
#else
    app_storage_ui(editor.text(),editor.cursor(),unsigned(menu.tab()),mesh_conversation(0),force,&nav);
#endif
}
void request_page(std::uint64_t before) {
#ifdef CARDPUTER_ADV_UI_V2
    history_newer=false;
#endif
    nav.history_before=before;page_pending=true;history_requested_records=app_storage_status().records;app_storage_page(before
#ifdef CARDPUTER_ADV_M5
    ,active_conversation()
#endif
    );}
void text(int x,int y,const char* value,int width=236) {
    char clipped[128]{};std::size_t size=0;
    while(value[size]) {auto end=TextEditor::next(value,size);if(end>=sizeof(clipped) || display.getStringWidth(value,end,true)>width)break;size=end;}
    std::memcpy(clipped,value,size);for(auto& c:clipped)if(c=='\n' || c=='\r' || c=='\t')c=' ';display.drawString(x,y,clipped);
}
void line(int y,const char* value){text(2,y,value);}
std::size_t line_end(const char* value,std::size_t start) {
    auto end=start;
    while(value[end] && value[end]!='\n') {auto next=TextEditor::next(value,end);if(display.getStringWidth(value+start,next-start,true)>236)break;end=next;}
    return end;
}
unsigned wrap_count(const char* value) {
    unsigned count=0;std::size_t at=0;
    while(value[at]){auto end=line_end(value,at);if(end==at && value[at]!='\n')break;at=end+(value[end]=='\n');++count;}return count;
}
void wrapped(const char* value,int top,int bottom,unsigned skip=0) {
    std::size_t at=0;unsigned row=0;
    while(value[at]) {
        auto end=line_end(value,at);if(end==at && value[at]!='\n')break;
        if(row>=skip) {
            auto y=top+int(row-skip)*kShellLine;if(y+kShellLine>bottom)break;
            char part[kMeshTextBytes+1]{};auto size=std::min<std::size_t>(end-at,sizeof(part)-1);std::memcpy(part,value+at,size);line(y,part);
        }
        ++row;at=end+(value[end]=='\n');
    }
}
void draw_editor(TextEditor& edit,int y) {
    std::size_t start=0;
    while(start<edit.cursor() && display.getStringWidth(edit.text()+start,edit.cursor()-start,true)>216)start=TextEditor::next(edit.text(),start);
    text(2,y,"> ",12);text(16,y,edit.text()+start,222);
    auto x=16+display.getStringWidth(edit.text()+start,edit.cursor()-start,true);
    display.drawHorizontalLine(x,y+15,4);
}
void draw_menu() {
    display.setRGB(palette().text);
    if(!menu.visible())return;
    display.drawHorizontalLine(0,117,240);
    for(unsigned i=0;i<4;++i) {
        auto active=i==unsigned(menu.tab());
        if(active){
#ifdef CARDPUTER_ADV_UI_V2
        display.setRGB(palette().active);
#endif
        display.setColor(WHITE);display.fillRect(i*60,118,60,17);
#ifdef CARDPUTER_ADV_UI_V2
        display.setRGB(palette().active_text);
#else
        display.setColor(BLACK);
#endif
        }
        auto width=display.getStringWidth(tab_names[i]);text(i*60+(60-width)/2,118,tab_names[i],58);
        display.setRGB(palette().text);display.setColor(WHITE);
    }
}
const char* delivery(const StoredRecord& r) {
    if(r.event.kind==MeshEventKind::Text)return "Recebida";
    switch(r.delivery) {
    case MeshEventKind::Transmitted:return "TX radio; sem ACK";
    case MeshEventKind::RoutingFeedback:return r.event.result==3 || r.event.result==8?"Timeout / sem resposta":r.event.result?"NAK / falha":"ACK recebido";
    case MeshEventKind::Error:return "Falha de envio";
    default:return "Fila; resultado desconhecido";
    }
}
void send_draft() {
    if(!editor.length()){m2_console_message("Digite a mensagem primeiro");return;}
    if(pending_tx){m2_console_message("Envio em gravacao; aguarde");return;}
    MeshCommand c;c.token=++token;std::strcpy(c.text,editor.text());
#ifdef CARDPUTER_ADV_M5
    c.channel=nav.chat_channel;c.expected_conversation=active_conversation();
    #ifdef CARDPUTER_ADV_M7
    c.destination=nav.chat_peer?nav.chat_peer:0xffffffff;
#endif
    if(c.expected_conversation && c.expected_conversation!=mesh_conversation(c.channel,nav.chat_peer)){m2_console_message("Reabra o canal antes de enviar");return;}
    if(!c.expected_conversation){m2_console_message("Escolha um canal ativo");return;}
#endif
    if(!mesh_submit(c)){m2_console_message("Nucleo/SD ou fila indisponivel");return;}
    std::strcpy(pending_text,editor.text());pending_tx=c.token;m2_console_message("Gravando antes de enviar");save(true);
}
void field_value(unsigned selected,char* out,std::size_t capacity) {
    auto s=mesh_snapshot();StoredRecord ui;app_storage_load_ui(ui);
    switch(selected) {
    case 0:std::snprintf(out,capacity,"%s",s.short_name);break;
    case 1:std::snprintf(out,capacity,"%s",s.long_name);break;
    case 2:std::snprintf(out,capacity,"%s",region_name(s.region));break;
    case 3:std::snprintf(out,capacity,"%s",preset_name(s.preset));break;
    case 4:std::snprintf(out,capacity,"%u",s.hop_limit);break;
    case 5:std::snprintf(out,capacity,"%u",s.frequency_slot);break;
    case 6:std::snprintf(out,capacity,"%d",ui.utc_offset);break;
    default:out[0]=0;break;
    }
}
void open_setting() {
#ifdef CARDPUTER_ADV_M8
    if(nav.settings_selection==12){theme_preview=nav.theme;settings_page=SettingsPage::Theme;return;}
    if(nav.settings_selection==13){settings_page=SettingsPage::Tools;return;}
#endif
    if(nav.settings_selection<7) {
        if(pending_config){m2_console_message("Aguarde configuracao anterior");return;}
        form.clear();constexpr std::size_t limits[]={4,39,16,16,1,5,4};form.set_limit(limits[nav.settings_selection]);
        char value[64];field_value(nav.settings_selection,value,sizeof(value));form.set(value,std::strlen(value));settings_page=SettingsPage::Edit;m2_console_message("Edite o valor deste campo");
    } else settings_page=nav.settings_selection==7?SettingsPage::Gps:nav.settings_selection==8?SettingsPage::Storage:nav.settings_selection==9?SettingsPage::Diagnostic:nav.settings_selection==10?SettingsPage::Help:SettingsPage::ClearConfirm;
}
bool number(long& value) {char* end=nullptr;value=std::strtol(form.text(),&end,10);return end!=form.text() && !*end;}
void submit_field() {
    auto s=mesh_snapshot();MeshCommand c;c.token=++token;
    c.region=s.region;c.preset=s.preset;c.hop_limit=s.hop_limit;c.frequency_slot=s.frequency_slot;
    bool valid=true;long value=0;
    if(nav.settings_selection<2) {
        c.kind=MeshCommandKind::SetOwner;
        std::memcpy(c.short_name,s.short_name,sizeof(c.short_name));std::memcpy(c.long_name,s.long_name,sizeof(c.long_name));
        auto max=nav.settings_selection==0?sizeof(c.short_name):sizeof(c.long_name);
        valid=form.length()<max;
        if(valid)std::strcpy(nav.settings_selection==0?c.short_name:c.long_name,form.text());
    } else if(nav.settings_selection<4) {
        c.kind=MeshCommandKind::SetRadio;valid=false;
        for(unsigned i=0;i<(nav.settings_selection==2?34:14);++i)if(!std::strcmp(form.text(),nav.settings_selection==2?region_names[i]:preset_names[i])) {if(nav.settings_selection==2)c.region=i;else c.preset=i;valid=true;break;}
    } else {
        valid=number(value);
        if(nav.settings_selection==6) {
            auto st=app_storage_status();
            if(!valid || value<-720 || value>840 || !st.ready || st.fault || !app_storage_offset(value)){m2_console_message("Fuso: -720 a +840; exige SD");return;}
            settings_page=SettingsPage::List;m2_console_message("Fuso na fila de gravacao");save(true);return;
        }
        c.kind=MeshCommandKind::SetRadio;
        valid=valid && value>=0 && value<=(nav.settings_selection==4?7:65535);
        if(valid){if(nav.settings_selection==4)c.hop_limit=value;else c.frequency_slot=value;}
    }
    if(!valid || !mesh_validate_command(c)){m2_console_message("Valor invalido; confira o campo");return;}
    if(!mesh_submit(c)){m2_console_message("Nucleo/SD ou fila indisponivel");return;}
    pending_config=c.token;settings_page=SettingsPage::List;m2_console_message("Salvando configuracao");
}
void cycle_field(bool backwards) {
    unsigned count=nav.settings_selection==2?34:14,index=0;
    for(unsigned i=0;i<count;++i)if(!std::strcmp(form.text(),nav.settings_selection==2?region_names[i]:preset_names[i])){index=i;break;}
    for(unsigned attempts=0;attempts<count;++attempts) {index=(index+(backwards?count-1:1))%count;if(nav.settings_selection!=2 || mesh_region_supported(index))break;}
    auto name=nav.settings_selection==2?region_names[index]:preset_names[index];form.set(name,std::strlen(name));
}
bool edit_key(TextEditor& edit,const KeyEvent& e) {
    switch(e.action) {
    case KeyAction::Text:if(!edit.insert(e.character))m2_console_message("Limite do campo atingido");return true;
    case KeyAction::Backspace:edit.backspace();return true;
    case KeyAction::Left:edit.left();return true;
    case KeyAction::Right:edit.right();return true;
    default:return false;
    }
}
#ifdef CARDPUTER_ADV_M5
void select_conversation(std::uint8_t channel) {
    if(channel>=chat_channel_count || !chat_channels[channel].role){m2_console_message("Canal desativado");return;}
    if(pending_tx){m2_console_message("Aguarde a gravacao do envio");return;}
    auto identity=mesh_conversation(channel);
    if(channel==nav.chat_channel && identity==active_conversation()){chat_page=ChatPage::List;return;}
    save(true);
    if(!app_storage_select(identity,channel)){m2_console_message("SD indisponivel para trocar");return;}
    switching_chat=true;page_pending=false;m2_console_message("Carregando conversa do SD");
}
void node_age(const MeshNode& node,char* out,std::size_t capacity) {
    auto s=mesh_snapshot();std::uint32_t age=0;bool known=false;
    if(node.heard_this_boot){age=std::uint32_t(millis()-node.heard_millis)/1000;known=true;}
    else if(s.gps_time && node.last_heard && node.last_heard<=s.utc){age=s.utc-node.last_heard;known=true;}
    if(!known)std::snprintf(out,capacity,"RX --");
    else if(age<60)std::snprintf(out,capacity,"RX %lus",static_cast<unsigned long>(age));
    else if(age<3600)std::snprintf(out,capacity,"RX %lum",static_cast<unsigned long>(age/60));
    else if(age<86400)std::snprintf(out,capacity,"RX %luh",static_cast<unsigned long>(age/3600));
    else std::snprintf(out,capacity,"RX %lud",static_cast<unsigned long>(age/86400));
}
void draw_conversations(int bottom) {
    char value[100];line(16,"CONVERSAS / Enter: abrir");auto first=(conversation_selection/4)*4u;
    for(unsigned i=first;i<chat_channel_count && i<first+4;++i){auto c=app_storage_conversation(i);
        if(chat_channels[i].role)std::snprintf(value,sizeof(value),"%c %u %s [%lu%s]",i==conversation_selection?'>':' ',i,chat_channels[i].name,static_cast<unsigned long>(c.unread),c.scanning?"+":"");
        else std::snprintf(value,sizeof(value),"%c %u Livre",i==conversation_selection?'>':' ',i);
        line(32+16*(i-first),value);
    }
    line(bottom-16,notice);
}
#endif
#ifdef CARDPUTER_ADV_UI_V2
void history_scroll(bool up){
    if(page_pending || !history_count)return;
    if(up){
        history_follow=false;
        if(nav.history_selection)--nav.history_selection;
        else {
            // Overlap the existing window: advancing one row never replaces
            // the visible conversation with a separate page of messages.
            history_direction=-1;history_anchor=history.front().sequence;history_previous_before=nav.history_before;
            request_page(history[history_count-1].sequence);
        }
    } else {
        if(nav.history_selection+1<history_count)++nav.history_selection;
        else if(nav.history_before){
            history_direction=1;history_anchor=history[history_count-1].sequence;history_previous_before=nav.history_before;
            history_newer=true;page_pending=true;history_requested_records=app_storage_status().records;
            app_storage_page_newer(history.front().sequence,active_conversation());
        }
        else history_follow=true;
    }
    if(!up && !nav.history_before && nav.history_selection+1==history_count)history_follow=true;
    auto rows=unsigned((shell_editor_y(menu.visible())-16-32)/16);
    if(nav.history_selection<nav.history_first)nav.history_first=nav.history_selection;
    else if(nav.history_selection>=nav.history_first+rows)nav.history_first=nav.history_selection-rows+1;
}
#endif
void chat_key(const KeyEvent& e) {
#ifdef CARDPUTER_ADV_M5
    if(chat_page==ChatPage::Conversations){
        if(e.action==KeyAction::Up && conversation_selection)--conversation_selection;
        else if(e.action==KeyAction::Down && conversation_selection+1<chat_channel_count)++conversation_selection;
        else if(e.action==KeyAction::Enter && !e.repeat)select_conversation(conversation_selection);
        else if(e.action==KeyAction::Escape)chat_page=ChatPage::List;
        return;
    }
#endif
    if(chat_page==ChatPage::Detail) {
#ifdef CARDPUTER_ADV_UI_V2
        display.setRGB(detail.event.kind==MeshEventKind::Queued?palette().sent:palette().received);
#endif
#ifdef CARDPUTER_ADV_M5
        if(e.action==KeyAction::Left || e.action==KeyAction::Right){detail_page^=1;return;}
#endif
        auto count=wrap_count(detail.event.text);
        if(e.action==KeyAction::Up && nav.detail_row)--nav.detail_row;
        else if(e.action==KeyAction::Down && nav.detail_row+1<count)++nav.detail_row;
        else if(e.action==KeyAction::Escape)chat_page=ChatPage::List;
        return;
    }
    if(chat_page==ChatPage::Compose) {
#ifdef CARDPUTER_ADV_UI_V2
        if(e.action==KeyAction::Up || e.action==KeyAction::Down){history_scroll(e.action==KeyAction::Up);return;}
#endif
        if(edit_key(editor,e))return;
        if(e.action==KeyAction::Escape){chat_page=ChatPage::List;save(true);}
        else if(e.action==KeyAction::Send && !e.repeat)send_draft();
        return;
    }
#ifdef CARDPUTER_ADV_UI_V2
    if(e.action==KeyAction::Text || e.action==KeyAction::Backspace || e.action==KeyAction::Send){m2_console_message("Enter: escrever; direita: detalhe");return;}
    if(e.action==KeyAction::Enter && !e.repeat){chat_page=ChatPage::Compose;return;}
    if(e.action==KeyAction::Right && !e.repeat) {
#else
    if(e.action==KeyAction::Text || e.action==KeyAction::Backspace || e.action==KeyAction::Left || e.action==KeyAction::Right) {chat_page=ChatPage::Compose;edit_key(editor,e);}
    else if(e.action==KeyAction::Enter && !e.repeat) {
#endif
        if(history_count && !page_pending) {detail=history[nav.history_selection];chat_page=ChatPage::Detail;
#ifdef CARDPUTER_ADV_M5
            detail_page=0;
#endif
            if(nav.detail_sequence!=detail.sequence){nav.detail_row=0;nav.detail_sequence=detail.sequence;}}
#ifndef CARDPUTER_ADV_UI_V2
        else chat_page=ChatPage::Compose;
#endif
    } else if(e.action==KeyAction::Send && !e.repeat) {chat_page=ChatPage::Compose;send_draft();}
    else if(e.action==KeyAction::Up && !page_pending) {
        #ifdef CARDPUTER_ADV_UI_V2
        history_scroll(true);
#else
        if(nav.history_selection)--nav.history_selection;
        else if(history_count){request_page(history[history_count-1].sequence);nav.history_selection=0;nav.history_first=0;}
#endif
    } else if(e.action==KeyAction::Down && !page_pending) {
        #ifdef CARDPUTER_ADV_UI_V2
        history_scroll(false);
#else
        if(nav.history_selection+1<history_count)++nav.history_selection;
        else {request_page(0);nav.history_selection=0;nav.history_first=0;}
#endif
    } else if(e.action==KeyAction::Escape) {
#ifdef CARDPUTER_ADV_M5
        chat_page=ChatPage::Conversations;conversation_selection=nav.chat_channel;m2_console_message("Setas: escolher; Enter: abrir");
#else
        menu.close();
#endif
    }
    auto rows=unsigned((shell_editor_y(menu.visible())-16-32)/16);
    if(nav.history_selection<nav.history_first)nav.history_first=nav.history_selection;
    else if(nav.history_selection>=nav.history_first+rows)nav.history_first=nav.history_selection-rows+1;
}
#ifdef CARDPUTER_ADV_M7_NODES
std::size_t nodes_current(MeshNode* out){auto count=mesh_nodes_find(nav.node_offset,nav.node_query,nav.node_favorites,out,4,node_total);if(!count && node_total && nav.node_offset){nav.node_offset=((node_total-1)/4)*4;count=mesh_nodes_find(nav.node_offset,nav.node_query,nav.node_favorites,out,4,node_total);}if(!node_total)nav.node_offset=0;nav.node_selection=count?std::min<std::size_t>(nav.node_selection,count-1):0;return count;}
#endif
void nodes_key(const KeyEvent& e) {
#ifdef CARDPUTER_ADV_M8
    if(trace_open){if(e.action==KeyAction::Up || e.action==KeyAction::Down)trace_export_notice=false;if(e.action==KeyAction::Text && (e.character=='e' || e.character=='E') && !e.repeat){trace_export_notice=true;if(!maintenance_submit(MaintenanceAction::Trace))m2_console_message("Rota/SD ocupado ou indisponivel");else m2_console_message("Exportando rota no SD");return;}if(e.action==KeyAction::Escape)trace_open=false;else if(e.action==KeyAction::Up && trace_row)--trace_row;else if(e.action==KeyAction::Down && trace_row<100)++trace_row;return;}
    if(node_open && e.action==KeyAction::Text && (e.character=='t' || e.character=='T') && !e.repeat){
        MeshCommand command;command.kind=MeshCommandKind::TraceRoute;command.destination=node_detail.number;command.token=++token;
        if(!mesh_submit(command)){node_notice=true;m2_console_message("Traceroute: radio/SD/fila ocupado");return;}
        trace_token=command.token;trace_row=0;trace_export_notice=false;trace_open=true;return;
    }
#endif
#ifdef CARDPUTER_ADV_M7_NODES
    if(node_search_open){if(e.action==KeyAction::Escape)node_search_open=false;else if(e.action==KeyAction::Enter && !e.repeat){std::strcpy(nav.node_query,node_search_editor.text());node_search_open=false;nav.node_offset=0;nav.node_selection=0;}else edit_key(node_search_editor,e);return;}
    if(!node_open && (e.action==KeyAction::Text || e.action==KeyAction::Right)){node_search_open=true;node_search_editor.set(e.action==KeyAction::Text?"":nav.node_query,std::strlen(nav.node_query));if(e.action==KeyAction::Text)node_search_editor.insert(e.character);return;}
    if(!node_open && e.action==KeyAction::Left){nav.node_favorites=!nav.node_favorites;nav.node_offset=0;nav.node_selection=0;return;}
    if(node_open && e.action==KeyAction::Text && (e.character=='f' || e.character=='F') && !e.repeat){node_notice=true;if(favorite_token){m2_console_message("Favorito: aguarde SD");return;}MeshCommand c;c.kind=MeshCommandKind::SetFavorite;c.token=++token;c.destination=node_detail.number;c.favorite=!node_detail.favorite;if(!mesh_submit(c)){m2_console_message("SD/fila indisponivel");return;}favorite_token=c.token;m2_console_message("Gravando favorito no SD");return;}
#endif
    if(node_open){
#ifdef CARDPUTER_ADV_M7
        if(e.action==KeyAction::Enter && !e.repeat){
            node_notice=true;
            if(pending_tx){m2_console_message("Aguarde gravacao do envio");return;}
            auto identity=mesh_conversation(nav.chat_channel,node_detail.number);
            if(!identity){m2_console_message("DM indisponivel: aguarde NodeInfo PKI");return;}
            save(true);if(!app_storage_select_peer(identity,nav.chat_channel,node_detail.number)){m2_console_message("SD indisponivel para DM");return;}
            switching_chat=true;page_pending=false;
#ifdef CARDPUTER_ADV_M8
            auto menu_was_visible=menu.visible();
#endif
            menu.select(ShellTab::Chat,millis());
#ifdef CARDPUTER_ADV_M8
            if(!menu_was_visible)menu.close();
#endif
            node_open=false;m2_console_message("Abrindo DM PKI");return;
        }
#endif
        if(e.action==KeyAction::Escape)node_open=false;else if(e.action==KeyAction::Up || e.action==KeyAction::Down)node_detail_page=(node_detail_page+(e.action==KeyAction::Up?
#ifdef CARDPUTER_ADV_M5
#ifdef CARDPUTER_ADV_M7_NODES
        4:1))%5;
#else
        2:1))%3;
#endif
#else
        1:1))%2;
#endif
        return;}
    MeshNode nodes[4];auto count=
#ifdef CARDPUTER_ADV_M7_NODES
        nodes_current(nodes);
#else
        mesh_nodes_page(nav.node_offset,nodes,4);
#endif
    if(e.action==KeyAction::Up) {if(nav.node_selection)--nav.node_selection;else if(nav.node_offset>=4){nav.node_offset-=4;nav.node_selection=3;}}
    else if(e.action==KeyAction::Down) {if(nav.node_selection+1<count)++nav.node_selection;else if(nav.node_offset+4<
#ifdef CARDPUTER_ADV_M7_NODES
        node_total
#else
        mesh_snapshot().nodes
#endif
        ){nav.node_offset+=4;nav.node_selection=0;}}
    else if(e.action==KeyAction::Enter && !e.repeat && count){node_detail=nodes[std::min<std::size_t>(nav.node_selection,count-1)];node_detail_page=0;node_open=true;
#ifdef CARDPUTER_ADV_M7
        node_notice=false;
#ifdef CARDPUTER_ADV_M7_NODES
        node_metrics=app_storage_node_metrics(node_detail.number);
        if(node_metrics.has_voltage!=node_detail.has_voltage || node_metrics.has_battery!=node_detail.has_battery || (node_metrics.has_voltage && node_metrics.voltage!=node_detail.voltage) || (node_metrics.has_battery && node_metrics.battery!=node_detail.battery))node_metrics.time=0;
#endif
#endif
}
    else if(e.action==KeyAction::Escape)menu.close();
}
void channels_key(const KeyEvent& e) {
#ifdef CARDPUTER_ADV_M6
    if(m6_channels_root()){if(e.action==KeyAction::Up && nav.channel_selection)--nav.channel_selection;else if(e.action==KeyAction::Down && nav.channel_selection<7)++nav.channel_selection;}
    m6_channels_key(e,nav.channel_selection);return;
#endif
    if(channel_open){if(e.action==KeyAction::Escape)channel_open=false;return;}
    if(e.action==KeyAction::Up && nav.channel_selection)--nav.channel_selection;
    else if(e.action==KeyAction::Down && nav.channel_selection<7)++nav.channel_selection;
    else if(e.action==KeyAction::Enter && !e.repeat)channel_open=true;
    else if(e.action==KeyAction::Escape)menu.close();
}
void settings_key(const KeyEvent& e) {
#ifdef CARDPUTER_ADV_M8
    if(maintenance_busy()){m2_console_message("Operacao SD em andamento");return;}
    if(settings_page==SettingsPage::Theme){
        if(e.action==KeyAction::Escape)settings_page=SettingsPage::List;
        else if(e.action==KeyAction::Up)theme_preview=(theme_preview+kThemeCount-1)%kThemeCount;
        else if(e.action==KeyAction::Down)theme_preview=(theme_preview+1)%kThemeCount;
        else if(e.action==KeyAction::Enter && !e.repeat){if(app_storage_status().fault || !app_storage_status().ready){m2_console_message("SD indisponivel: tema nao salvo");return;}nav.theme=theme_preview;settings_page=SettingsPage::List;save(true);m2_console_message("Tema na fila de gravacao SD");}
        return;
    }
    if(settings_page==SettingsPage::Tools){
        if(e.action==KeyAction::Escape)settings_page=SettingsPage::List;
        else if(e.action==KeyAction::Up && tools_selection)--tools_selection;
        else if(e.action==KeyAction::Down && tools_selection<2)++tools_selection;
        else if(e.action==KeyAction::Enter && !e.repeat){
            if(!tools_selection){if(!maintenance_submit(MaintenanceAction::Diagnostic))m2_console_message("Diagnostico: SD indisponivel");else m2_console_message("Exportando diagnostico no SD");}
            else if(tools_selection==1)settings_page=SettingsPage::BackupConfirm;
            else {restore_count=0;restore_selection=0;for(unsigned i=0;i<kBackupSlots;++i)if(core_storage_backup_exists(i))restore_slots[restore_count++]=i;settings_page=SettingsPage::RestoreList;}
        }
        return;
    }
    if(settings_page==SettingsPage::BackupConfirm){
        if(e.action==KeyAction::Escape)settings_page=SettingsPage::Tools;
        else if(e.action==KeyAction::Enter && !e.repeat){settings_page=SettingsPage::Tools;if(!maintenance_submit(MaintenanceAction::Backup))m2_console_message("Backup: SD indisponivel");else m2_console_message("Criando backup verificado");}
        return;
    }
    if(settings_page==SettingsPage::RestoreList){
        if(e.action==KeyAction::Escape)settings_page=SettingsPage::Tools;
        else if(e.action==KeyAction::Up && restore_selection)--restore_selection;
        else if(e.action==KeyAction::Down && restore_selection+1<restore_count)++restore_selection;
        else if(e.action==KeyAction::Enter && !e.repeat && restore_count)settings_page=SettingsPage::RestoreConfirm;
        return;
    }
    if(settings_page==SettingsPage::RestoreConfirm){
        if(e.action==KeyAction::Escape)settings_page=SettingsPage::RestoreList;
        else if(e.action==KeyAction::Enter && !e.repeat){if(pending_tx || pending_config || favorite_token || trace_token || mesh_trace_snapshot().tracking){m2_console_message("Aguarde TX/config/traceroute");return;}save(true);settings_page=SettingsPage::Tools;if(!maintenance_submit(MaintenanceAction::Restore,restore_slots[restore_selection]))m2_console_message("Restauracao: SD/rascunho indisponivel");else m2_console_message("Validando backup; aguarde");}
        return;
    }
#endif
    if(settings_page==SettingsPage::Edit) {
        if((e.action==KeyAction::Up || e.action==KeyAction::Down) && (nav.settings_selection==2 || nav.settings_selection==3))cycle_field(e.action==KeyAction::Up);
        else if(edit_key(form,e)){}
        else if(e.action==KeyAction::Enter && !e.repeat)submit_field();
        else if(e.action==KeyAction::Escape)settings_page=SettingsPage::List;
        return;
    }
    if(settings_page==SettingsPage::ClearConfirm) {
        if(e.action==KeyAction::Escape)settings_page=SettingsPage::List;
        else if(e.action==KeyAction::Enter && !e.repeat) {if(pending_tx)m2_console_message("Aguarde o envio pendente");else {editor.clear();settings_page=SettingsPage::List;save(true);m2_console_message("Rascunho limpo");}}
        return;
    }
    if(settings_page!=SettingsPage::List) {
        if(e.action==KeyAction::Escape)settings_page=SettingsPage::List;
        else if(settings_page==SettingsPage::Gps && (e.action==KeyAction::Up || e.action==KeyAction::Down))gps_page^=1;
        else if(settings_page==SettingsPage::Help && (e.action==KeyAction::Up || e.action==KeyAction::Down))help_page^=1;
        return;
    }
    if(e.action==KeyAction::Up && nav.settings_selection)--nav.settings_selection;
    else if(e.action==KeyAction::Down && nav.settings_selection+1<kSettingCount)++nav.settings_selection;
    else if(e.action==KeyAction::Enter && !e.repeat)open_setting();
    else if(e.action==KeyAction::Escape)menu.close();
}
void draw_chat(int bottom) {
#ifdef CARDPUTER_ADV_M5
    if(chat_page==ChatPage::Conversations){draw_conversations(bottom);return;}
    if(switching_chat){line(16,"Carregando conversa do SD");return;}
    char value[kMeshTextBytes+32];auto unread=app_storage_conversation(nav.chat_channel).unread;
    std::snprintf(value,sizeof(value),"#%u %s %s",nav.chat_channel,nav.chat_channel<chat_channel_count?chat_channels[nav.chat_channel].name:"--",unread?"+RX":"");
#ifdef CARDPUTER_ADV_M7
    if(nav.chat_peer)std::snprintf(value,sizeof(value),"DM !%08lx PKI%s",static_cast<unsigned long>(nav.chat_peer),new_rx?" +RX":"");
#endif
    line(16,value);
#else
    auto s=mesh_snapshot();char value[kMeshTextBytes+32];std::snprintf(value,sizeof(value),"#%.20s %s",s.channel_name,new_rx?"+RX":"");
#ifdef CARDPUTER_ADV_M7
    if(nav.chat_peer)std::snprintf(value,sizeof(value),"DM !%08lx PKI%s",static_cast<unsigned long>(nav.chat_peer),new_rx?" +RX":"");
#endif
    line(16,value);
#endif
    if(chat_page==ChatPage::Detail) {
#ifdef CARDPUTER_ADV_UI_V2
        display.setRGB(detail.event.kind==MeshEventKind::Queued?palette().sent:palette().received);
#endif
#ifdef CARDPUTER_ADV_M5
        if(detail_page){
            if(detail.utc_valid){StoredRecord ui;app_storage_load_ui(ui);auto minute=shell_local_minute(detail.utc,ui.utc_offset);std::snprintf(value,sizeof(value),"Local %02u:%02u (GPS)",minute/60,minute%60);}else std::strcpy(value,"Hora GPS desconhecida");line(32,value);
            if(detail.event.kind==MeshEventKind::Text)std::snprintf(value,sizeof(value),"RSSI %.0f SNR %.1f",detail.event.rssi,detail.event.snr);else std::strcpy(value,"Texto enviado neste canal");line(48,value);
            if(detail.event.kind==MeshEventKind::Text && detail.event.hop_start && detail.event.hop_start>=detail.event.hop_limit)std::snprintf(value,sizeof(value),"Saltos observados %u",detail.event.hop_start-detail.event.hop_limit);else std::strcpy(value,"Saltos --");line(64,value);
            std::snprintf(value,sizeof(value),"Pacote %08lx",static_cast<unsigned long>(detail.event.packet_id));line(80,value);line(bottom-16,"Esq/dir: texto; Esc: voltar");return;
        }
#endif
        std::snprintf(value,sizeof(value),"%s !%08lx",detail.event.kind==MeshEventKind::Text?"RX":"TX",static_cast<unsigned long>(detail.event.node));line(32,value);
        line(48,delivery(detail));wrapped(detail.event.text,64,bottom-16,nav.detail_row);line(bottom-16,
#ifdef CARDPUTER_ADV_M5
        "Cima/baixo: ler; dir: dados"
#else
        "Setas: ler; Esc: voltar"
#endif
        );return;
    }
    auto y=bottom-16;
    {
        auto rows=unsigned((y-16-32)/16);auto first=nav.history_first;
#ifdef CARDPUTER_ADV_UI_V2
        if(history_count){if(nav.history_selection<first)first=nav.history_selection;else if(nav.history_selection>=first+rows)first=nav.history_selection-rows+1;first=std::min<unsigned>(first,history_count>rows?history_count-rows:0);}else first=0;
        nav.history_first=first;
#endif
        auto visible=std::min<std::size_t>(rows,history_count>first?history_count-first:0);
        auto history_top=y-16-int(visible)*16;
        for(unsigned i=first;i<history_count && i-first<rows;++i) {
            auto& r=history[i];std::snprintf(value,sizeof(value),"%c %s: %s",i==nav.history_selection?'>':' ',
#ifdef CARDPUTER_ADV_M5
            history_names[i],
#else
            r.event.kind==MeshEventKind::Text?"RX":"EU",
#endif
            r.event.text);
#ifdef CARDPUTER_ADV_UI_V2
            display.setRGB(r.event.kind==MeshEventKind::Queued?palette().sent:palette().received);
            if(i==nav.history_selection){display.setRGB(palette().panel);display.fillRect(0,history_top+16*(i-first),240,16);display.setRGB(r.event.kind==MeshEventKind::Queued?palette().sent:palette().received);}
#endif
            line(history_top+16*(i-first),value);
#ifdef CARDPUTER_ADV_UI_V2
            display.setRGB(palette().text);
#endif
        }
        if(!history_count)line(32,page_pending?"Lendo historico...":"Enter: escrever");
        if(chat_page==ChatPage::Compose){std::snprintf(value,sizeof(value),"%u/%u | Fn+Enter: enviar",unsigned(editor.length()),unsigned(editor.limit()));line(y-16,value);}
        else line(y-16,notice);
    }
#ifdef CARDPUTER_ADV_UI_V2
    display.setRGB(palette().text);
    if(chat_page==ChatPage::Compose)draw_editor(editor,y);else line(y,editor.length()?"Enter: escrever (rascunho)":"Enter: escrever; dir: detalhe");
#else
    draw_editor(editor,y);
#endif
}
#ifdef CARDPUTER_ADV_M5
void draw_nodes(int bottom) {
#ifdef CARDPUTER_ADV_M8
    if(trace_open){auto trace=mesh_trace_snapshot();char label[64];line(16,"TRACEROUTE / manual");std::snprintf(label,sizeof(label),"!%08lx%s",static_cast<unsigned long>(trace.node),trace.tracking?" consultando":"");line(32,label);
        if(trace_token){line(48,"Aguardando nucleo");line(64,notice);}
        else if(trace.available){auto rows=wrap_count(trace.text);if(trace_row>=rows)trace_row=rows?rows-1:0;wrapped(trace.text,48,bottom-16,trace_row);}
        else line(48,notice);
        if(maintenance_busy())line(bottom-16,"Exportando rota no SD");else if(trace_export_notice)line(bottom-16,notice);else if(trace.truncated)line(bottom-16,"Truncada; E: salvar; Esc: sair");else line(bottom-16,"Setas; E: salvar; Esc: voltar");return;}
#endif
    char value[120],age[32];
#ifdef CARDPUTER_ADV_M7_NODES
    if(node_search_open){line(16,"BUSCA / NOME OU ID");line(48,"Enter: aplicar; Esc: cancelar");draw_editor(node_search_editor,bottom-16);return;}
    if(node_open)std::snprintf(value,sizeof(value),"NO %u/5 %s | F: favorito",node_detail_page+1,node_detail.favorite?"*":"");
    else std::snprintf(value,sizeof(value),"NOS%s %s",nav.node_favorites?" *":"",nav.node_query);
    line(16,value);
#else
    line(16,node_open?
#ifdef CARDPUTER_ADV_M8
    "NO / F: favorito T: rota"
#else
    "NO / DETALHE"
#endif
    :"NOS / Enter: detalhe");
#endif
    if(node_open){std::snprintf(value,sizeof(value),"!%08lx %s",static_cast<unsigned long>(node_detail.number),node_detail.short_name);line(32,value);node_age(node_detail,age,sizeof(age));
        if(node_detail_page==0){
#ifdef CARDPUTER_ADV_M7
            line(80,mesh_conversation(nav.chat_channel,node_detail.number)?"Enter: abrir DM PKI":"DM: aguarde NodeInfo PKI");
#endif
            line(48,node_detail.has_user?node_detail.long_name:"Nome desconhecido");std::snprintf(value,sizeof(value),"%s / %s",age,node_detail.heard_this_boot?"neste boot":node_detail.heard_known?"cache SD":"origem --");line(64,value);}
        else if(node_detail_page==1){if(node_detail.heard_this_boot || node_detail.last_heard)std::snprintf(value,sizeof(value),"SNR %.1f dB%s",node_detail.snr,node_detail.heard_this_boot?"":" (cache)");else std::strcpy(value,"SNR --");line(48,value);
            if(node_detail.has_hops)std::snprintf(value,sizeof(value),"Hops %u | %s",node_detail.hops,age);else std::snprintf(value,sizeof(value),"Hops -- | %s",age);line(64,value);
            if(node_detail.has_battery && node_detail.battery>100){if(node_detail.has_voltage)std::snprintf(value,sizeof(value),"Energia externa %.2fV (cache)",node_detail.voltage);else std::strcpy(value,"Energia externa (cache)");}
            else if(node_detail.has_battery && node_detail.has_voltage)std::snprintf(value,sizeof(value),"Bateria %u%% %.2fV (cache)",node_detail.battery,node_detail.voltage);else if(node_detail.has_voltage)std::snprintf(value,sizeof(value),"Bateria %.2fV (cache)",node_detail.voltage);else if(node_detail.has_battery)std::snprintf(value,sizeof(value),"Bateria %u%% (cache)",node_detail.battery);else std::strcpy(value,"Bateria --");line(80,value);
        }
#ifdef CARDPUTER_ADV_M7_NODES
        else if(node_detail_page==3){auto d=node_distance(mesh_snapshot(),node_detail,millis());if(!d.valid){line(48,"Fixes atuais indisponiveis");line(64,"GPS local + posicao do no");line(80,"Remoto: validade de 15min");}else {if(d.metres<1000)std::snprintf(value,sizeof(value),"Distancia %.0f m",d.metres);else std::snprintf(value,sizeof(value),"Distancia %.2f km",d.metres/1000);line(48,value);if(d.has_bearing)std::snprintf(value,sizeof(value),"Rumo %.0f graus (norte)",d.bearing);else std::strcpy(value,"Rumo -- (mesmo ponto)");line(64,value);line(80,"Aproximacao por GPS");}}
        else if(node_detail_page==4){auto s=mesh_snapshot();if(!node_detail.has_voltage && !node_detail.has_battery){line(48,"Sem telemetria do no");}else if(!node_metrics.time){line(48,"Cache: hora desconhecida");line(64,"Reabra apos nova telemetria");}else if(!s.gps_time || node_metrics.time>s.utc){line(48,"Hora sem referencia GPS");}else {time_t epoch=node_metrics.time;auto date=std::gmtime(&epoch);if(date){std::strftime(value,sizeof(value),"RX UTC %d/%m/%y %H:%M",date);line(48,value);}auto minutes=(s.utc-node_metrics.time)/60;std::snprintf(value,sizeof(value),"Recebida ha %lum%s",static_cast<unsigned long>(minutes),s.utc-node_metrics.time>900?" (antiga)":"");line(64,value);line(80,"Hora RX, nao da medicao");}}
#endif
        else {if(node_detail.has_position){std::snprintf(value,sizeof(value),"Lat %.5f",node_detail.latitude/1e7);line(48,value);std::snprintf(value,sizeof(value),"Lon %.5f Alt %ldm",node_detail.longitude/1e7,static_cast<long>(node_detail.altitude));line(64,value);
                auto s=mesh_snapshot();if(s.gps_time && node_detail.position_time && node_detail.position_time<=s.utc)std::snprintf(value,sizeof(value),"Posicao cache ha %lum",static_cast<unsigned long>((s.utc-node_detail.position_time)/60));else std::strcpy(value,"Posicao cache / idade --");line(80,value);
            }else line(48,"Posicao desconhecida");}
    } else {MeshNode nodes[4];auto count=
#ifdef CARDPUTER_ADV_M7_NODES
        nodes_current(nodes);
#else
        mesh_nodes_page(nav.node_offset,nodes,4);
#endif
        for(unsigned i=0;i<count;++i){node_age(nodes[i],age,sizeof(age));char name[16];if(nodes[i].has_user && nodes[i].short_name[0])std::snprintf(name,sizeof(name),"%s",nodes[i].short_name);else std::snprintf(name,sizeof(name),"!%08lx",static_cast<unsigned long>(nodes[i].number));
#ifdef CARDPUTER_ADV_M7_NODES
            if(nodes[i].favorite && std::strlen(name)+1<sizeof(name))std::strcat(name,"*");
#endif
            if(nodes[i].heard_this_boot || nodes[i].last_heard)std::snprintf(value,sizeof(value),"%c %s %s S%.1f",i==nav.node_selection?'>':' ',name,age,nodes[i].snr);else std::snprintf(value,sizeof(value),"%c %s RX -- S--",i==nav.node_selection?'>':' ',name);line(32+16*i,value);}
        if(!count)line(32,"Nenhum no disponivel");}
#ifdef CARDPUTER_ADV_M7
    if(node_open && node_notice){line(bottom-16,notice);return;}
#endif
#ifdef CARDPUTER_ADV_M7_NODES
    line(bottom-16,node_open?"Setas: pagina; Enter: DM":"Digite: busca; <-: favoritos");
#else
    line(bottom-16,node_open?"Setas: pagina; Esc: voltar":"Setas: selecionar / pagina");
#endif
}
#else
void draw_nodes(int bottom) {
#ifdef CARDPUTER_ADV_M8
    if(trace_open){auto trace=mesh_trace_snapshot();char label[64];line(16,"TRACEROUTE / manual");std::snprintf(label,sizeof(label),"!%08lx%s",static_cast<unsigned long>(trace.node),trace.tracking?" consultando":"");line(32,label);
        if(trace_token){line(48,"Aguardando nucleo");line(64,notice);}
        else if(trace.available){auto rows=wrap_count(trace.text);if(trace_row>=rows)trace_row=rows?rows-1:0;wrapped(trace.text,48,bottom-16,trace_row);}
        else line(48,notice);
        if(maintenance_busy())line(bottom-16,"Exportando rota no SD");else if(trace_export_notice)line(bottom-16,notice);else if(trace.truncated)line(bottom-16,"Truncada; E: salvar; Esc: sair");else line(bottom-16,"Setas; E: salvar; Esc: voltar");return;}
#endif
    char value[120];line(16,node_open?
#ifdef CARDPUTER_ADV_M8
    "NO / F: favorito T: rota"
#else
    "NO / DETALHE"
#endif
    :"NOS / Enter: detalhe");
    if(node_open) {
        std::snprintf(value,sizeof(value),"!%08lx %s",static_cast<unsigned long>(node_detail.number),node_detail.short_name);line(32,value);
        if(!node_detail_page){line(48,node_detail.long_name);line(64,node_detail.heard_this_boot?"Ouvido neste boot":node_detail.heard_known?"Cache no SD":"RX desconhecido");}
        else {std::snprintf(value,sizeof(value),"SNR %.1f | ultimo RX %lu",node_detail.snr,static_cast<unsigned long>(node_detail.last_heard));line(48,value);if(node_detail.has_position)std::snprintf(value,sizeof(value),"%.5f, %.5f",node_detail.latitude/1e7,node_detail.longitude/1e7);else std::snprintf(value,sizeof(value),"Posicao --");line(64,value);}
    } else {
        MeshNode nodes[4];auto count=
#ifdef CARDPUTER_ADV_M7_NODES
        nodes_current(nodes);
#else
        mesh_nodes_page(nav.node_offset,nodes,4);
#endif
        for(unsigned i=0;i<count;++i){std::snprintf(value,sizeof(value),"%c %-4s !%08lx",i==nav.node_selection?'>':' ',nodes[i].short_name,static_cast<unsigned long>(nodes[i].number));line(32+16*i,value);}
        if(!count)line(32,"Nenhum no disponivel");
    }
#ifdef CARDPUTER_ADV_M7
    if(node_open && node_notice){line(bottom-16,notice);return;}
#endif
#ifdef CARDPUTER_ADV_M7_NODES
    line(bottom-16,node_open?"Setas: pagina; Enter: DM":"Digite: busca; <-: favoritos");
#else
    line(bottom-16,node_open?"Setas: pagina; Esc: voltar":"Setas: selecionar / pagina");
#endif
}
#endif
void draw_channels(int bottom) {
#ifdef CARDPUTER_ADV_M6
    m6_channels_draw(display,bottom,nav.channel_selection);return;
#endif
    MeshChannel channels[8];auto count=mesh_channels_page(0,channels,8);char value[100];line(16,"CANAIS / consulta");
    if(channel_open) {
        auto index=nav.channel_selection;
        if(index<count) {auto& ch=channels[index];std::snprintf(value,sizeof(value),"Slot %u: %s",index,ch.role?ch.name:"Livre / desativado");line(32,value);line(48,ch.role==1?"Primario":ch.role==2?"Secundario":"Desativado");line(64,ch.encrypted?"Criptografia ativa":"Sem chave ativa");}
        else line(32,"Slot livre");
    } else {
        unsigned first=(nav.channel_selection/4)*4;
        for(unsigned i=first;i<first+4 && i<8;++i){std::snprintf(value,sizeof(value),"%c %u %s",i==nav.channel_selection?'>':' ',i,i<count && channels[i].role?channels[i].name:"Livre");line(32+16*(i-first),value);}
    }
    line(bottom-16,channel_open?"Somente leitura; Esc: voltar":"Setas; Enter: detalhes");
}
void draw_settings(int bottom) {
    char value[128];auto s=mesh_snapshot();auto st=app_storage_status();line(16,"AJUSTES");
    if(settings_page==SettingsPage::List) {
        unsigned first=(nav.settings_selection/4)*4;
        for(unsigned i=first;i<first+4 && i<kSettingCount;++i){std::snprintf(value,sizeof(value),"%c %s",i==nav.settings_selection?'>':' ',setting_names[i]);line(32+16*(i-first),value);}
        line(bottom-16,notice);
    }
#ifdef CARDPUTER_ADV_M8
    else if(settings_page==SettingsPage::Theme){
        line(32,theme(theme_preview).name);line(48,app_storage_status().fault?"SD indisponivel: nao salvo":"Texto e fundo: alto contraste");
        display.setRGB(palette().sent);line(64,"EU: mensagem enviada");display.setRGB(palette().received);line(80,"NO: mensagem recebida");display.setRGB(palette().text);
        line(bottom-16,"Setas; Enter: salvar; Esc: sair");
    } else if(settings_page==SettingsPage::Tools){
        const char* items[]={"Exportar diagnostico","Criar backup","Restaurar backup"};
        for(unsigned i=0;i<3;++i){std::snprintf(value,sizeof(value),"%c %s",i==tools_selection?'>':' ',items[i]);line(32+16*i,value);}
        line(bottom-16,notice);
    } else if(settings_page==SettingsPage::BackupConfirm){line(32,"Backup da configuracao");line(48,"Inclui identidade e chaves");line(64,"Historico/rascunhos ficam");line(bottom-16,"Enter: criar; Esc: cancelar");}
    else if(settings_page==SettingsPage::RestoreList){
        if(!restore_count)line(32,"Nenhum backup no SD");
        unsigned first=(restore_selection/4)*4;
        for(unsigned i=first;i<first+4 && i<restore_count;++i){std::snprintf(value,sizeof(value),"%c config-%03u",i==restore_selection?'>':' ',restore_slots[i]);line(32+16*(i-first),value);}
        line(bottom-16,"Enter: selecionar; Esc: voltar");
    } else if(settings_page==SettingsPage::RestoreConfirm){
        std::snprintf(value,sizeof(value),"Restaurar config-%03u?",restore_slots[restore_selection]);line(32,value);line(48,"Restaura chaves e rede");line(64,"Reinicia; historico fica");line(bottom-16,"Enter: restaurar; Esc: voltar");
    }
#endif
    else if(settings_page==SettingsPage::Edit) {
        line(32,setting_names[nav.settings_selection]);draw_editor(form,48);
        std::snprintf(value,sizeof(value),"%u/%u bytes",unsigned(form.length()),unsigned(form.limit()));line(64,value);line(80,notice);
        line(bottom-16,"Enter: salvar; Esc: cancelar");
    } else if(settings_page==SettingsPage::Gps) {
#ifdef CARDPUTER_ADV_M5
        if(gps_page){if(s.gps_fix){std::snprintf(value,sizeof(value),"Lat %.5f",s.latitude/1e7);line(32,value);std::snprintf(value,sizeof(value),"Lon %.5f",s.longitude/1e7);line(48,value);std::snprintf(value,sizeof(value),"Altitude %ldm",static_cast<long>(s.altitude));line(64,value);
                if(s.gps_fix_millis)std::snprintf(value,sizeof(value),"Fix ha %lus",static_cast<unsigned long>(std::uint32_t(millis()-s.gps_fix_millis)/1000));else std::strcpy(value,"Idade do fix --");line(80,value);
            }else {line(32,"GPS sem fix posicional");line(48,"Coordenadas --");line(64,"Hora pode estar valida");}
            line(bottom-16,"Setas: pagina; Esc: voltar");return;}
#endif
        if(s.gps_time)std::snprintf(value,sizeof(value),"UTC %02lu:%02lu:%02lu",static_cast<unsigned long>((s.utc/3600)%24),static_cast<unsigned long>((s.utc/60)%60),static_cast<unsigned long>(s.utc%60));else std::strcpy(value,"Hora UTC --");line(32,value);
        StoredRecord ui;app_storage_load_ui(ui);auto minute=shell_local_minute(s.utc,ui.utc_offset);
        if(s.gps_time)std::snprintf(value,sizeof(value),"Local %02u:%02u",minute/60,minute%60);else std::strcpy(value,"Hora local --");line(48,value);
        std::snprintf(value,sizeof(value),"Offset UTC %+d min",ui.utc_offset);line(80,value);
        std::snprintf(value,sizeof(value),"Fix %s | satelites %u",s.gps_fix?"OK":"--",s.satellites);line(64,value);line(bottom-16,"Setas: pagina; Esc: voltar");
    } else if(settings_page==SettingsPage::Storage) {
        std::snprintf(value,sizeof(value),"SD %s | registros %llu",s.storage_fault||st.fault?"ERRO":"OK",static_cast<unsigned long long>(st.records));line(32,value);
        std::snprintf(value,sizeof(value),"Fila %lu | CRC %lu",static_cast<unsigned long>(st.dropped),static_cast<unsigned long>(st.corrupt));line(48,value);line(64,st.recovered?"Recuperacao detectada":"Persistencia ativa");line(80,"Limite: 49152 registros");line(bottom-16,"Texto + telemetria + eventos");
    } else if(settings_page==SettingsPage::Diagnostic) {
        std::snprintf(value,sizeof(value),"ADV %s !%08lx",APP_VERSION,static_cast<unsigned long>(s.node));line(32,value);
        std::snprintf(value,sizeof(value),"TX %lu RX %lu erro %lu",static_cast<unsigned long>(s.tx),static_cast<unsigned long>(s.rx),static_cast<unsigned long>(s.rx_errors));line(48,value);
        std::snprintf(value,sizeof(value),"Heap minimo %lu",static_cast<unsigned long>(s.heap_min));line(64,value);
#ifdef CARDPUTER_ADV_M8
        std::snprintf(value,sizeof(value),"Loop max %lums",static_cast<unsigned long>(s.loop_max_ms));line(80,value);
#endif
        line(bottom-16,"Esc: voltar");
    } else if(settings_page==SettingsPage::ClearConfirm) {line(32,"Apagar texto do rascunho?");line(48,"Historico sera preservado");line(bottom-16,"Enter: limpar; Esc: cancelar");}
    else {
        if(!help_page){line(32,"Tab: abre menu / troca aba");line(48,"Menu some apos 5 segundos");line(64,"Setas: navegar / cursor");}
        else {line(32,"Enter no Chat: escrever");line(48,"Fn+Enter: enviar mensagem");line(64,"Esc: voltar sem apagar");}
        line(bottom-16,"Setas: pagina; Esc: voltar");
    }
}
void poll_events() {
    MeshEvent e;while(mesh_poll_event(e)) {
#ifdef CARDPUTER_ADV_M8
        if(trace_token && e.token==trace_token && (e.kind==MeshEventKind::Configured || e.kind==MeshEventKind::Error)){trace_token=0;m2_console_message(e.text);if(e.kind==MeshEventKind::Error){trace_open=false;node_notice=true;}continue;}
#endif
#ifdef CARDPUTER_ADV_M7_NODES
        if(favorite_token && e.token==favorite_token && (e.kind==MeshEventKind::Configured || e.kind==MeshEventKind::Error)){favorite_token=0;if(e.kind==MeshEventKind::Configured && node_detail.number==e.node)node_detail.favorite=e.result;node_notice=true;m2_console_message(e.kind==MeshEventKind::Configured?"Favorito salvo no SD":e.text);continue;}
#endif
#ifdef CARDPUTER_ADV_M6
        m6_channels_event(e);
#endif
        if(e.token==pending_tx && pending_tx && (e.kind==MeshEventKind::Queued || e.kind==MeshEventKind::Error)) {
            if(e.kind==MeshEventKind::Queued && !std::strcmp(editor.text(),pending_text))editor.clear();
            pending_tx=0;pending_text[0]=0;save(true);
        }
        if(e.token==pending_config && pending_config && (e.kind==MeshEventKind::Configured || e.kind==MeshEventKind::Error))pending_config=0;
        if(e.kind==MeshEventKind::Text) {
#ifdef CARDPUTER_ADV_M5
            #ifdef CARDPUTER_ADV_M7
            auto peer=e.destination && e.destination!=0xffffffff?e.node:0;
            if(mesh_conversation(e.channel,peer)!=active_conversation()){
                if(peer){char incoming[64];std::snprintf(incoming,sizeof(incoming),"DM recebida de !%08lx",static_cast<unsigned long>(peer));m2_console_message(incoming);}
                continue;
            }
#else
            if(e.channel!=nav.chat_channel || (e.destination && e.destination!=0xffffffff))continue;
#endif
#endif
            if(new_rx<99)++new_rx;
            if(menu.tab()==ShellTab::Chat
#ifdef CARDPUTER_ADV_UI_V2
            && history_follow
#endif
            && (chat_page==ChatPage::List || chat_page==ChatPage::Compose) && !nav.history_before && !page_pending)request_page(0);
        } else if(e.kind==MeshEventKind::Queued) {last_packet=e.packet_id;m2_console_message("Fila local salva no SD");if(
#ifdef CARDPUTER_ADV_UI_V2
        history_follow &&
#endif
        (chat_page==ChatPage::List || chat_page==ChatPage::Compose) && !nav.history_before && !page_pending)request_page(0);}
        else if(e.kind==MeshEventKind::Transmitted && e.packet_id==last_packet)m2_console_message("TX radio; entrega nao confirmada");
        else if(e.kind==MeshEventKind::RoutingFeedback && e.packet_id==last_packet)m2_console_message(e.result?"Feedback: falha":"ACK recebido");
        else if(e.kind==MeshEventKind::Configured)m2_console_message("Configuracao salva/aplicada");
        else if(e.kind==MeshEventKind::Error)m2_console_message(e.text);
    }
}
}
void m2_console_message(const char* value){std::snprintf(notice,sizeof(notice),"%s",value);redraw=true;}
void m2_console_begin() {
    #ifdef CARDPUTER_ADV_M8
    maintenance_begin();
#endif
    frame=millis()-200;menu.begin(millis());
#ifdef CARDPUTER_ADV_M8
    menu.close();
#endif
    pinMode(VTFT_LEDA,OUTPUT);digitalWrite(VTFT_LEDA,HIGH);display_ready=display.init();
    if(display_ready){display.flipScreenVertically();display.setRGB(palette().text);display.setFont(Adv_Plain_12);}
}
void m2_console_key(const KeyEvent& e) {
    if(e.action==KeyAction::None)return;
#ifdef CARDPUTER_ADV_M5
    if(switching_chat || app_storage_status().switching){m2_console_message("Carregando conversa; aguarde");return;}
#endif
    auto now=millis();redraw|=menu.tick(now);
    redraw=true;
    if(e.action==KeyAction::Tab || e.action==KeyAction::BackTab) {
        auto blocked=menu.tab()==ShellTab::Settings && (settings_page==SettingsPage::ClearConfirm
#ifdef CARDPUTER_ADV_M8
        || settings_page==SettingsPage::Theme || settings_page==SettingsPage::BackupConfirm || settings_page==SettingsPage::RestoreConfirm || maintenance_busy()
#endif
        );
        if(menu.navigate(e.action==KeyAction::BackTab,now,blocked)) {if(menu.tab()==ShellTab::Chat)new_rx=0;save(true);}
        return;
    }
    switch(menu.tab()) {
    case ShellTab::Chat:chat_key(e);break;
    case ShellTab::Nodes:nodes_key(e);break;
    case ShellTab::Channels:channels_key(e);break;
    case ShellTab::Settings:settings_key(e);break;
    }
    save(e.action==KeyAction::Escape || e.action==KeyAction::Send || e.action==KeyAction::Enter);
}
void m2_console_tick() {
    auto now=millis();redraw|=menu.tick(now);
#ifdef CARDPUTER_ADV_M8
    static bool recovery_shown=false;
    if(!recovery_shown && core_storage_recovery()){recovery_shown=true;menu.begin(now,ShellTab::Settings);menu.close();settings_page=SettingsPage::Tools;tools_selection=2;m2_console_message("Recuperacao: restaurar backup");}
#endif
    if(!restored && app_storage_status().ready) {
        StoredRecord saved;auto tab=ShellTab::Chat;
#ifdef CARDPUTER_ADV_M5
        chat_channel_count=mesh_channels_page(0,chat_channels,8);unsigned primary=0;
        std::uint64_t identities[8]{};for(unsigned i=0;i<chat_channel_count;++i){if(chat_channels[i].role==1)primary=i;if(chat_channels[i].role)identities[i]=mesh_conversation(i);}
        nav.chat_channel=primary;
#endif
        if(app_storage_load_ui(saved)) {
            if(!saved.conversation || saved.conversation==mesh_conversation(
#ifdef CARDPUTER_ADV_M5
                saved.navigation.conversations?saved.navigation.chat_channel:nav.chat_channel,saved.navigation.chat_peer
#else
                0
#endif
                ))editor.set(saved.event.text,saved.cursor);
            else m2_console_message("Rascunho de outro canal no SD");
            if(saved.navigation.shell){
#ifdef CARDPUTER_ADV_M5
                auto channel=nav.chat_channel;nav=saved.navigation;if(!nav.conversations)nav.chat_channel=channel;
#else
                nav=saved.navigation;
#endif
                tab=ShellTab(saved.view);}
            else tab=saved.view==1?ShellTab::Nodes:saved.view==3?ShellTab::Chat:ShellTab::Settings;
        }
        menu.begin(now,tab);
#ifdef CARDPUTER_ADV_M8
        menu.close();
#endif
        restored=true;redraw=true;
#ifdef CARDPUTER_ADV_UI_V2
        history_follow=!nav.history_before;
#endif
        nav.node_offset=(std::min<std::uint32_t>(nav.node_offset,mesh_snapshot().nodes?mesh_snapshot().nodes-1:0)/4)*4;
#ifdef CARDPUTER_ADV_M5
        if(nav.chat_channel>=chat_channel_count || !chat_channels[nav.chat_channel].role){nav.chat_channel=primary;editor.clear();nav.history_before=0;nav.history_selection=0;nav.history_first=0;nav.read_sequence=0;}
        chat_identity=nav.chat_peer?mesh_conversation(nav.chat_channel,nav.chat_peer):identities[nav.chat_channel];
        if(!chat_identity && nav.chat_peer){nav.chat_peer=0;chat_identity=identities[nav.chat_channel];editor.clear();m2_console_message("PKI mudou; rascunho antigo preservado");}
        if(saved.conversation && saved.conversation!=chat_identity){editor.clear();nav.read_sequence=0;nav.history_before=0;nav.history_selection=0;nav.history_first=0;}
        app_storage_bind_channels(identities,8);save(true);
#endif
        request_page(nav.history_before);
    }
#ifdef CARDPUTER_ADV_M5
    if(switching_chat && app_storage_status().fault){switching_chat=false;page_pending=false;m2_console_message("Falha SD: conversa preservada");}
    if(switching_chat && !app_storage_status().switching){StoredRecord saved;if(app_storage_load_ui(saved)){nav=saved.navigation;chat_identity=saved.conversation;editor.set(saved.event.text,saved.cursor);chat_page=ChatPage::List;new_rx=0;switching_chat=false;
#ifdef CARDPUTER_ADV_UI_V2
history_direction=0;history_follow=!nav.history_before;
#endif
request_page(nav.history_before);redraw=true;}}
#endif
    poll_events();
#ifdef CARDPUTER_ADV_M8
    MaintenanceResult result;if(maintenance_poll(result)){
        if(result.action==MaintenanceAction::Restore && result.success){esp_restart();return;}
        char message[120];if(!result.success)std::strcpy(message,"Falha SD ou backup incompativel");
        else if(result.action==MaintenanceAction::Backup)std::snprintf(message,sizeof(message),"Backup config-%03u salvo (%lums)",result.slot,static_cast<unsigned long>(result.duration_ms));
        else std::snprintf(message,sizeof(message),result.action==MaintenanceAction::Trace?"Rota salva no SD (%lums)":"Diagnostico salvo no SD (%lums)",static_cast<unsigned long>(result.duration_ms));
        m2_console_message(message);if(trace_open && result.action==MaintenanceAction::Trace)trace_row=0;
    }
#endif
#ifdef CARDPUTER_ADV_M6
    m6_channels_tick();
    auto channels_revision=mesh_channels_revision();
    if(restored && channels_seen_revision!=channels_revision && !switching_chat){channels_seen_revision=channels_revision;auto previous=active_conversation();chat_channel_count=mesh_channels_page(0,chat_channels,8);std::uint64_t ids[8]{};unsigned primary=0;for(unsigned i=0;i<chat_channel_count;++i){if(chat_channels[i].role)ids[i]=mesh_conversation(i);if(chat_channels[i].role==1)primary=i;}app_storage_bind_channels(ids,8);
        unsigned selected=nav.chat_channel;if(selected>=8 || !ids[selected])selected=primary;
        if(!nav.chat_peer && ids[selected]!=previous){save(true);if(!pending_tx && app_storage_select(ids[selected],selected)){switching_chat=true;page_pending=false;}}
    }
#endif
    if(page_pending && !app_storage_status().busy) {
        std::array<StoredRecord,4> incoming{};
        auto incoming_count=app_storage_page_read(incoming.data(),incoming.size());
        page_pending=false;history_records=history_requested_records;redraw=true;
#ifdef CARDPUTER_ADV_UI_V2
        std::reverse(incoming.begin(),incoming.begin()+incoming_count);
        std::size_t selected=incoming_count?std::min<std::size_t>(nav.history_selection,incoming_count-1):0;
        bool accept=true;
        if(history_direction){
            bool found=false;
            if(history_direction<0){for(unsigned i=0;i<incoming_count;++i)if(incoming[i].sequence<history_anchor){selected=i;found=true;}}
            else {for(unsigned i=0;i<incoming_count;++i)if(incoming[i].sequence>history_anchor){selected=i;found=true;break;}}
            // At an endpoint keep the current window, selection and scroll offset.
            if(!found){accept=false;nav.history_before=history_previous_before;}
        } else if(history_follow && incoming_count)selected=incoming_count-1;
        if(accept){
            history=incoming;history_count=incoming_count;nav.history_selection=selected;
            if(history_newer && history_count)nav.history_before=app_storage_page_latest()?0:history[history_count-1].sequence+1;
            if(history_direction>0 && !nav.history_before && selected+1==history_count)history_follow=true;
            auto rows=unsigned((shell_editor_y(menu.visible())-16-32)/16);
            nav.history_first=nav.history_selection>=rows?nav.history_selection-rows+1:0;
        }
        history_direction=0;history_newer=false;save();
#else
        history=incoming;history_count=incoming_count;
        nav.history_selection=history_count?std::min<std::size_t>(nav.history_selection,history_count-1):0;
#endif
#ifdef CARDPUTER_ADV_M5
        for(unsigned i=0;i<history_count;++i){if(history[i].event.kind==MeshEventKind::Queued)std::strcpy(history_names[i],"EU");else if(!mesh_node_name(history[i].event.node,history_names[i],sizeof(history_names[i])))std::snprintf(history_names[i],sizeof(history_names[i]),"!%08lx",static_cast<unsigned long>(history[i].event.node));}
#endif
    }

    if(restored
#ifdef CARDPUTER_ADV_UI_V2
    && history_follow
#endif
    && menu.tab()==ShellTab::Chat && (chat_page==ChatPage::List || chat_page==ChatPage::Compose) && !nav.history_before && !page_pending && app_storage_status().records!=history_records)request_page(0);
#ifdef CARDPUTER_ADV_M5
    if(restored && !switching_chat && menu.tab()==ShellTab::Chat && (chat_page==ChatPage::List || chat_page==ChatPage::Compose) && !page_pending && !nav.history_before && history_count){auto sequence=std::max_element(history.begin(),history.begin()+history_count,[](const StoredRecord& a,const StoredRecord& b){return a.sequence<b.sequence;})->sequence;if(sequence>nav.read_sequence){nav.read_sequence=sequence;app_storage_mark_read(active_conversation(),sequence);}}
#endif
    if(!display_ready || (!redraw && std::uint32_t(now-frame)<100))return;frame=now;redraw=false;
    #ifdef CARDPUTER_ADV_M8
    display.setBackground(palette().background);
#endif
    display.clear();display.setRGB(palette().text);display.setColor(WHITE);char value[96];auto s=mesh_snapshot();auto st=app_storage_status();
    StoredRecord ui;app_storage_load_ui(ui);auto minute=shell_local_minute(s.utc,ui.utc_offset);
#ifdef CARDPUTER_ADV_M8
    if(menu.visible()){
        char battery[8];if(s.battery_percent_known && s.battery_percent<=100)std::snprintf(battery,sizeof(battery),"%u%%",unsigned(s.battery_percent));else std::strcpy(battery,"--%");
        auto battery_x=238-display.getStringWidth(battery);
        if(s.gps_time)std::snprintf(value,sizeof(value),"RF %s SD %s %02lu:%02lu",s.radio_ready?"OK":"--",s.storage_fault||st.fault?"!":st.dropped?"FILA":st.recovered?"REC":st.ready?"OK":"--",static_cast<unsigned long>(minute/60),static_cast<unsigned long>(minute%60));
        else std::snprintf(value,sizeof(value),"RF %s SD %s --:--",s.radio_ready?"OK":"--",s.storage_fault||st.fault?"!":st.ready?"OK":"--");
        text(2,0,value,battery_x-6);text(battery_x,0,battery,238-battery_x);
    }
    auto bottom=shell_bottom(menu.visible());
#else
    if(s.gps_time)std::snprintf(value,sizeof(value),"RF %s SD %s %02lu:%02lu %.1fV",s.radio_ready?"OK":"--",s.storage_fault||st.fault?"!":st.dropped?"FILA":st.recovered?"REC":st.ready?"OK":"--",static_cast<unsigned long>(minute/60),static_cast<unsigned long>(minute%60),s.battery_mv/1000.f);
    else std::snprintf(value,sizeof(value),"RF %s SD %s --:-- %.1fV",s.radio_ready?"OK":"--",s.storage_fault||st.fault?"!":st.ready?"OK":"--",s.battery_mv/1000.f);
#ifdef CARDPUTER_ADV_UI_V2
    display.setRGB(palette().panel);display.fillRect(0,0,240,16);display.setRGB(palette().text);
#endif
    line(0,value);auto bottom=shell_bottom(menu.visible());
#endif
    switch(menu.tab()) {
    case ShellTab::Chat:draw_chat(bottom);break;
    case ShellTab::Nodes:draw_nodes(bottom);break;
    case ShellTab::Channels:draw_channels(bottom);break;
    case ShellTab::Settings:draw_settings(bottom);break;
    }
    draw_menu();display.display();
}
}
#endif

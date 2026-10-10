#include "cardputer_adv/m2_console.hpp"
#include "cardputer_adv/app_storage.hpp"
#include "cardputer_adv/mesh_backend.hpp"
#include <ST7789Spi.h>
#include <cassert>
#include <deque>
#ifdef CARDPUTER_ADV_M8
#include "cardputer_adv/maintenance.hpp"
#include "cardputer_adv/core_storage.hpp"
#endif
namespace {
cardputer_adv::StoredRecord saved;
cardputer_adv::StorageStatus status;
cardputer_adv::MeshSnapshot snapshot;
std::deque<cardputer_adv::MeshEvent> events;
std::vector<cardputer_adv::MeshCommand> commands;
unsigned saves=0;
std::array<cardputer_adv::StoredRecord,8> contexts{};
std::array<cardputer_adv::ConversationStatus,8> conversations{};
std::uint64_t page_identity=0;
std::uint8_t selected_channel=0;
std::uint32_t selected_peer=0;
bool pki_ready=true;
#ifdef CARDPUTER_ADV_M7_NODES
bool node_favorites[2]{};bool favorite_failure=false;
#endif
#ifdef CARDPUTER_ADV_M6
unsigned active_slots=2;std::uint32_t channel_revision=1;
#endif
std::vector<cardputer_adv::StoredRecord> messages(2);
bool continuous_fixture=false,fixture_newer=false,fixture_latest=true;std::uint64_t fixture_boundary=0;
void tick(std::uint32_t time){fake_millis=time;cardputer_adv::m2_console_tick();}
void key(cardputer_adv::KeyAction action,char ch=0){++fake_millis;cardputer_adv::m2_console_key({action,ch,false});cardputer_adv::m2_console_tick();}
bool contains(const char* value){for(auto& s:ST7789Spi::strings)if(s.find(value)!=std::string::npos)return true;return false;}
}
namespace cardputer_adv {
#ifdef CARDPUTER_ADV_M8
TraceSnapshot fake_trace;bool maintenance_in_progress=false;MaintenanceAction last_action;unsigned maintenance_requests=0;
TraceSnapshot mesh_trace_snapshot(){return fake_trace;}
bool maintenance_begin(){return true;}
bool maintenance_busy(){return maintenance_in_progress;}
bool maintenance_submit(MaintenanceAction action,unsigned){if(status.fault)return false;last_action=action;++maintenance_requests;return true;}
bool maintenance_poll(MaintenanceResult&){return false;}
bool core_storage_backup_exists(unsigned i){return i==0 || i==3;}
#endif
StorageStatus app_storage_status(){return status;}
bool app_storage_load_ui(StoredRecord& out){out=saved;return true;}
void app_storage_ui(const char* draft,std::uint16_t cursor,std::uint8_t view,std::uint64_t conversation,bool,const UiNavigation* nav){std::strcpy(saved.event.text,draft);saved.cursor=cursor;saved.view=view;saved.conversation=conversation;if(nav)saved.navigation=*nav;++saves;}
bool app_storage_offset(std::int16_t value){saved.utc_offset=value;return true;}
void app_storage_page(std::uint64_t before,std::uint64_t identity){page_identity=identity;status.busy=true;fixture_boundary=before;fixture_newer=false;}
bool app_storage_page_latest(){return fixture_latest;}
void app_storage_page_newer(std::uint64_t after,std::uint64_t identity){app_storage_page(after,identity);fixture_newer=true;}
std::size_t app_storage_page_read(StoredRecord* out,std::size_t capacity){
 if(continuous_fixture){std::vector<StoredRecord> matches;for(auto r:messages)if((fixture_newer?r.sequence>fixture_boundary:!fixture_boundary || r.sequence<fixture_boundary))matches.push_back(r);std::sort(matches.begin(),matches.end(),[](const StoredRecord& a,const StoredRecord& b){return a.sequence>b.sequence;});fixture_latest=!fixture_boundary || (fixture_newer && matches.size()<=4);if(fixture_newer && matches.size()>4)matches.erase(matches.begin(),matches.end()-4);auto count=std::min<std::size_t>({capacity,matches.size(),4});for(unsigned i=0;i<count;++i)out[i]=matches[i];return count;}
 auto count=std::min(capacity,messages.size());for(std::size_t i=0;i<count;++i)out[i]=messages[i];for(std::size_t i=0;i<count;++i){out[i].conversation=page_identity;if(page_identity==56)std::strcpy(out[i].event.text,"Texto no secundario");}return count;}
std::uint64_t mesh_conversation(std::uint8_t channel,std::uint32_t peer){return peer?(pki_ready?1000+peer:0):55+channel;}
bool app_storage_bind_channels(const std::uint64_t* ids,std::size_t count){for(unsigned i=0;i<count;++i)conversations[i].identity=ids[i];return true;}
ConversationStatus app_storage_conversation(std::uint8_t channel){return conversations[channel];}
bool app_storage_select(std::uint64_t identity,std::uint8_t channel){contexts[saved.navigation.chat_channel]=saved;selected_channel=channel;selected_peer=0;status.switching=true;page_identity=identity;return true;}
#ifdef CARDPUTER_ADV_M7
bool app_storage_select_peer(std::uint64_t identity,std::uint8_t channel,std::uint32_t peer){auto ok=app_storage_select(identity,channel);selected_peer=peer;return ok;}
#endif
#ifdef CARDPUTER_ADV_M7_NODES
NodeMetricsStamp app_storage_node_metrics(std::uint32_t node){NodeMetricsStamp s;s.node=node;s.time=snapshot.utc-120;s.has_battery=node==0x1235;s.battery=101;return s;}
#endif
void app_storage_mark_read(std::uint64_t identity,std::uint64_t sequence){for(auto& c:conversations)if(c.identity==identity){c.read=sequence;c.unread=0;}saved.navigation.read_sequence=sequence;}
MeshSnapshot mesh_snapshot(){return snapshot;}
bool mesh_node_name(std::uint32_t,char* name,std::size_t capacity){std::snprintf(name,capacity,"ANA");return true;}
bool mesh_submit(const MeshCommand& c){if(!mesh_validate_command(c))return false;commands.push_back(c);
#ifdef CARDPUTER_ADV_M7_NODES
if(c.kind==MeshCommandKind::SetFavorite){MeshEvent e;e.token=c.token;e.node=c.destination;e.result=c.favorite;e.kind=favorite_failure?MeshEventKind::Error:MeshEventKind::Configured;if(!favorite_failure)node_favorites[c.destination-0x1234]=c.favorite;std::strcpy(e.text,"SD favorito falhou");events.push_back(e);}
#endif
return true;}
bool mesh_poll_event(MeshEvent& out){if(events.empty())return false;out=events.front();events.pop_front();return true;}
std::size_t mesh_nodes_page(std::size_t offset,MeshNode* out,std::size_t capacity){if(offset>=2)return 0;auto count=std::min<std::size_t>(2-offset,capacity);for(std::size_t i=0;i<count;++i){out[i]={};out[i].number=0x1234+offset+i;
#ifdef CARDPUTER_ADV_M7_NODES
out[i].has_user=true;out[i].favorite=node_favorites[offset+i];out[i].has_position=true;out[i].position_time=snapshot.utc;out[i].latitude=snapshot.latitude+10000;out[i].longitude=snapshot.longitude;std::strcpy(out[i].long_name,offset+i?"Beta":"Alpha");
#endif
if(offset+i){out[i].has_battery=true;out[i].battery=101;}std::strcpy(out[i].short_name,offset+i?"B":"A");}return count;}
std::size_t mesh_channels_page(std::size_t,MeshChannel* out,std::size_t capacity){auto count=std::min<std::size_t>(capacity,8);for(std::size_t i=0;i<count;++i){out[i]={};out[i].index=i;if(i<
#ifdef CARDPUTER_ADV_M6
active_slots
#else
2
#endif
){out[i].role=i?2:1;out[i].encrypted=true;std::strcpy(out[i].name,i==2?"Terceiro":i?"Secundario":"LongFast");}}return count;}
}
#ifdef CARDPUTER_ADV_M6
namespace cardputer_adv {
std::uint32_t mesh_channels_revision(){return channel_revision;}
void m6_channels_key(const KeyEvent&,std::uint8_t){}void m6_channels_draw(ST7789Spi&,int,std::uint8_t){}void m6_channels_tick(){}void m6_channels_event(const MeshEvent&){}bool m6_channels_root(){return true;}
}
#endif
void finish_switch(){auto old=saved;saved=selected_peer?cardputer_adv::StoredRecord{}:contexts[selected_channel];saved.kind=cardputer_adv::StoreKind::Ui;saved.navigation.shell=true;saved.navigation.conversations=true;saved.navigation.chat_channel=selected_channel;saved.navigation.chat_peer=selected_peer;saved.conversation=cardputer_adv::mesh_conversation(selected_channel,selected_peer);saved.utc_offset=old.utc_offset;saved.view=0;status.switching=false;tick(fake_millis+1);status.busy=false;tick(fake_millis+101);}
int main(int argc,char** argv){
    using namespace cardputer_adv;
    status.ready=true;status.records=2;snapshot.ready=true;snapshot.radio_ready=true;snapshot.storage_ready=true;snapshot.gps_time=true;snapshot.utc=1800000000;snapshot.battery_mv=4050;snapshot.nodes=2;snapshot.region=26;snapshot.node=2;
    std::strcpy(snapshot.channel_name,"LongFast");std::strcpy(snapshot.short_name,"ADV");std::strcpy(snapshot.long_name,"Cardputer");
    saved.kind=StoreKind::Ui;saved.view=3;std::strcpy(saved.event.text,"rascunho");saved.cursor=8;saved.conversation=55;
    messages[0].sequence=2;messages[0].event.kind=MeshEventKind::Text;messages[0].event.node=0x1234;std::strcpy(messages[0].event.text,"Mensagem recebida no ADV");
    messages[1].sequence=1;messages[1].event.kind=MeshEventKind::Queued;std::strcpy(messages[1].event.text,"Mensagem enviada");
#ifdef CARDPUTER_ADV_M8
    if(argc>1 && !std::strcmp(argv[1],"--continuous-chat")){
        continuous_fixture=true;messages.clear();
        for(unsigned i=12;i>0;--i){StoredRecord r;r.sequence=i;r.event.kind=i%2?MeshEventKind::Text:MeshEventKind::Queued;r.event.node=0x1234;std::snprintf(r.event.text,sizeof(r.event.text),"history-%02u",i);messages.push_back(r);}
        status.records=12;saved.view=0;saved.navigation.shell=true;saved.navigation.conversations=true;
        fake_millis=1000;m2_console_begin();tick(1001);status.busy=false;tick(1101);
        auto selected=[&](unsigned number){char text[40];std::snprintf(text,sizeof(text),"> %s: history-%02u",number%2?"ANA":"EU",number);assert(contains(text));assert(!contains("Lendo historico"));};
        auto settle_scroll=[&](){if(status.busy){assert(contains("history-"));status.busy=false;tick(fake_millis+101);}};
        selected(12);key(KeyAction::Tab);selected(12);key(KeyAction::Enter);key(KeyAction::Text,'x');auto cursor=saved.cursor;
        for(unsigned i=11;i>=1;--i){key(KeyAction::Up);settle_scroll();selected(i);for(unsigned visible=std::min(i,10u);visible<std::min(i,10u)+3;++visible){char body[24];std::snprintf(body,sizeof(body),"history-%02u",visible);assert(contains(body));}assert(saved.cursor==cursor && !std::strcmp(saved.event.text,"rascunhox"));}
        key(KeyAction::Up);settle_scroll();selected(1);
        // A received message cannot pull the reader away from older history.
        StoredRecord extra;extra.sequence=13;extra.event.kind=MeshEventKind::Text;extra.event.node=0x1234;std::strcpy(extra.event.text,"history-13");messages.insert(messages.begin(),extra);status.records=13;MeshEvent incoming=extra.event;events.push_back(incoming);tick(fake_millis+101);selected(1);assert(!status.busy);
        for(unsigned i=2;i<=13;++i){key(KeyAction::Down);settle_scroll();selected(i);for(unsigned visible=i>3?i-2:1;visible<=(i>3?i:3);++visible){char body[24];std::snprintf(body,sizeof(body),"history-%02u",visible);assert(contains(body));}assert(saved.cursor==cursor && !std::strcmp(saved.event.text,"rascunhox"));}
        key(KeyAction::Down);settle_scroll();selected(13);assert(saved.navigation.history_before==0);
        key(KeyAction::Escape);key(KeyAction::Right);assert(contains("history-13"));return 0;
    }
    if(argc>1 && !std::strcmp(argv[1],"--m8-chat")){
        saved.view=0;saved.navigation.shell=true;saved.navigation.conversations=true;snapshot.battery_percent=75;snapshot.battery_percent_known=true;
        fake_millis=1000;m2_console_begin();tick(1001);status.busy=false;tick(1101);
        auto header=[](){for(auto& d:ST7789Spi::draws)if(d.y==0)return true;return false;};
        assert(!header());key(KeyAction::Tab);assert(saved.view==0 && contains("75%") && contains("RF OK SD OK"));
        bool aligned=false;for(auto& d:ST7789Spi::draws)if(d.y==0 && d.text=="75%")aligned=d.x>200;assert(aligned);
        for(auto p:{0,100}){snapshot.battery_percent=p;tick(fake_millis+101);assert(contains(p?"100%":"0%"));}
        snapshot.battery_percent_known=false;tick(fake_millis+101);assert(contains("--%"));snapshot.battery_percent_known=true;snapshot.battery_percent=75;
        for(auto& d:ST7789Spi::draws)if(d.y==0)assert(d.text.find('V')==std::string::npos);
        key(KeyAction::Enter);key(KeyAction::Text,'x');assert(contains("Mensagem recebida") && contains("Mensagem enviada"));
        unsigned copies=0;for(auto& d:ST7789Spi::draws)if(d.text.find("rascunhox")!=std::string::npos){++copies;assert(d.y==101);}assert(copies==1);
        int rx_y=-1,tx_y=-1;for(auto& d:ST7789Spi::draws){if(d.text.find("Mensagem recebida")!=std::string::npos)rx_y=d.y;if(d.text.find("Mensagem enviada")!=std::string::npos)tx_y=d.y;}assert(rx_y>tx_y && tx_y>=32);
        auto cursor=saved.cursor;key(KeyAction::Up);assert(saved.navigation.history_selection==0 && saved.cursor==cursor && !std::strcmp(saved.event.text,"rascunhox"));key(KeyAction::Down);assert(saved.navigation.history_selection==1);
        MeshEvent incoming;incoming.kind=MeshEventKind::Text;incoming.node=0x1234;std::strcpy(incoming.text,"Novo RX durante composicao");events.push_back(incoming);messages[0].sequence=3;std::strcpy(messages[0].event.text,incoming.text);status.records=3;tick(fake_millis+101);assert(status.busy);status.busy=false;tick(fake_millis+101);assert(contains("Novo RX durante") && saved.cursor==cursor && !std::strcmp(saved.event.text,"rascunhox"));
        tick(6102);assert(!header());copies=0;for(auto& d:ST7789Spi::draws)if(d.text.find("rascunhox")!=std::string::npos){++copies;assert(d.y==119);}assert(copies==1);
        key(KeyAction::Escape);assert(!std::strcmp(saved.event.text,"rascunhox") && !contains("rascunhox"));key(KeyAction::Tab);assert(header() && saved.view==0);
        key(KeyAction::Tab);assert(saved.view==1);tick(fake_millis+5000);assert(!header());key(KeyAction::Enter);key(KeyAction::Enter);assert(status.switching);finish_switch();assert(saved.view==0 && !header());return 0;
    }
    if(argc>1 && !std::strcmp(argv[1],"--m8-recovery")){
        status.ready=false;snapshot.ready=false;fake_recovery=true;fake_millis=1000;m2_console_begin();tick(1001);assert(contains("Recuperacao") && contains("Restaurar backup"));key(KeyAction::Enter);assert(contains("config-000"));key(KeyAction::Enter);assert(contains("Reinicia"));key(KeyAction::Enter);assert(last_action==MaintenanceAction::Restore && maintenance_requests==1);return 0;
    }
    if(argc>1 && !std::strcmp(argv[1],"--m8")){
        saved.navigation.shell=true;saved.navigation.conversations=true;saved.navigation.settings_selection=12;saved.navigation.theme=1;
        fake_millis=1000;m2_console_begin();tick(1001);status.busy=false;tick(1101);key(KeyAction::Enter);assert(contains("Noite ambar"));
        key(KeyAction::Down);assert(contains("Claro") && saved.navigation.theme==1);key(KeyAction::Tab);assert(saved.view==3);key(KeyAction::Escape);assert(saved.navigation.theme==1);
        key(KeyAction::Enter);key(KeyAction::Down);key(KeyAction::Enter);assert(saved.navigation.theme==2 && !std::strcmp(saved.event.text,"rascunho"));
        key(KeyAction::Enter);key(KeyAction::Down);status.fault=true;key(KeyAction::Enter);assert(saved.navigation.theme==2);status.fault=false;key(KeyAction::Escape);
        key(KeyAction::Down);key(KeyAction::Enter);assert(contains("Exportar diagnostico"));key(KeyAction::Enter);assert(last_action==MaintenanceAction::Diagnostic && maintenance_requests==1);
        key(KeyAction::Down);key(KeyAction::Enter);assert(contains("Inclui identidade e chaves"));key(KeyAction::Escape);assert(maintenance_requests==1);key(KeyAction::Enter);key(KeyAction::Enter);assert(last_action==MaintenanceAction::Backup && maintenance_requests==2);
        key(KeyAction::Down);key(KeyAction::Enter);assert(contains("config-000") && contains("config-003"));key(KeyAction::Down);key(KeyAction::Enter);assert(contains("Restaurar config-003?") && contains("Reinicia"));key(KeyAction::Escape);assert(maintenance_requests==2);key(KeyAction::Enter);key(KeyAction::Enter);assert(last_action==MaintenanceAction::Restore && maintenance_requests==3);
        key(KeyAction::Escape);key(KeyAction::BackTab);key(KeyAction::BackTab);assert(saved.view==1);key(KeyAction::Enter);key(KeyAction::Text,'t');assert(commands.back().kind==MeshCommandKind::TraceRoute && commands.back().destination==0x1234 && contains("Aguardando nucleo"));
        MeshEvent event;event.kind=MeshEventKind::Configured;event.token=commands.back().token;events.push_back(event);fake_trace.available=true;fake_trace.node=0x1234;std::strcpy(fake_trace.text,"ADV > ANA\nANA > ADV");tick(fake_millis+101);assert(contains("ADV > ANA") && contains("ANA > ADV"));key(KeyAction::Text,'e');assert(last_action==MaintenanceAction::Trace && maintenance_requests==4);key(KeyAction::Escape);
        key(KeyAction::Text,'t');event.kind=MeshEventKind::Error;event.token=commands.back().token;std::strcpy(event.text,"Traceroute ocupado");events.push_back(event);tick(fake_millis+101);assert(contains("Traceroute ocupado"));return 0;
    }
#endif
#ifdef CARDPUTER_ADV_UI_V2
    if(argc>1 && !std::strcmp(argv[1],"--ui-v2")){saved.view=0;saved.navigation.shell=true;saved.navigation.conversations=true;fake_millis=1000;m2_console_begin();tick(1001);status.busy=false;tick(1101);key(KeyAction::Text,'x');key(KeyAction::Backspace);key(KeyAction::Send);assert(commands.empty() && !std::strcmp(saved.event.text,"rascunho"));key(KeyAction::Enter);key(KeyAction::Text,'x');key(KeyAction::Send);assert(commands.size()==1 && !std::strcmp(commands[0].text,"rascunhox"));MeshEvent queued;queued.kind=MeshEventKind::Queued;queued.token=commands[0].token;queued.packet_id=90;events.push_back(queued);tick(fake_millis+101);status.busy=false;tick(fake_millis+101);key(KeyAction::Escape);key(KeyAction::Right);assert(contains("Recebida"));key(KeyAction::Escape);key(KeyAction::Enter);key(KeyAction::Text,'y');key(KeyAction::Escape);key(KeyAction::Text,'z');assert(!std::strcmp(saved.event.text,"y"));key(KeyAction::Enter);key(KeyAction::Text,'z');assert(!std::strcmp(saved.event.text,"yz"));return 0;}
#endif
#ifdef CARDPUTER_ADV_M7_NODES
    if(argc>1 && !std::strcmp(argv[1],"--nodes")){
        saved.view=0;saved.navigation.shell=true;saved.navigation.conversations=true;fake_millis=1000;m2_console_begin();tick(1001);status.busy=false;tick(1101);key(KeyAction::Tab);assert(saved.view==1);
        key(KeyAction::Text,'a');key(KeyAction::Text,'l');key(KeyAction::Enter);assert(!std::strcmp(saved.navigation.node_query,"al") && contains("A RX") && !contains("B RX"));
        key(KeyAction::Right);key(KeyAction::Backspace);key(KeyAction::Escape);assert(!std::strcmp(saved.navigation.node_query,"al"));
        key(KeyAction::Enter);key(KeyAction::Text,'f');assert(commands.back().kind==MeshCommandKind::SetFavorite && commands.back().favorite && contains("Favorito salvo"));
        key(KeyAction::Escape);key(KeyAction::Left);assert(saved.navigation.node_favorites && contains("A* RX"));
        key(KeyAction::Enter);favorite_failure=true;key(KeyAction::Text,'f');assert(contains("SD favorito falhou") && node_favorites[0]);favorite_failure=false;key(KeyAction::Text,'f');assert(!node_favorites[0]);key(KeyAction::Escape);assert(contains("Nenhum no"));key(KeyAction::Left);
        key(KeyAction::Right);key(KeyAction::Backspace);key(KeyAction::Backspace);key(KeyAction::Enter);assert(!saved.navigation.node_query[0]);key(KeyAction::Down);key(KeyAction::Enter);
        key(KeyAction::Up);assert(contains("RX UTC") && contains("Recebida ha 2m"));ST7789Spi::capture("m7-telemetry");
        key(KeyAction::Up);assert(contains("Fixes atuais indisponiveis"));snapshot.gps_fix=true;snapshot.gps_fix_millis=fake_millis;snapshot.latitude=0;snapshot.longitude=0;tick(fake_millis+101);assert(contains("Distancia") && contains("Rumo"));ST7789Spi::capture("m7-distance");return 0;
    }
#endif
#ifndef CARDPUTER_ADV_M7_NODES
    (void)argv;
#endif
#ifdef CARDPUTER_ADV_M7
    if(argc>1){saved.view=0;saved.navigation.shell=true;saved.navigation.conversations=true;saved.navigation.chat_peer=0x1234;saved.conversation=1000+0x1234;std::strcpy(saved.event.text,"private reboot");saved.cursor=7;fake_millis=1000;m2_console_begin();tick(1001);status.busy=false;tick(1101);assert(contains("DM !00001234 PKI") && saved.navigation.chat_peer==0x1234 && !std::strcmp(saved.event.text,"private reboot") && saved.cursor==7);key(KeyAction::Send);assert(commands.back().destination==0x1234 && commands.back().expected_conversation==1000+0x1234);return 0;}
#else
    (void)argc;
#endif
    fake_millis=1000;m2_console_begin();m2_console_tick();status.busy=false;tick(1101);assert(contains("#0 LongFast"));
    tick(5999);assert(ST7789Spi::pixels[120*240+1]);tick(6000);assert(!ST7789Spi::pixels[120*240+1]);
    key(KeyAction::Escape);assert(contains("CONVERSAS"));key(KeyAction::Down);key(KeyAction::Enter);assert(status.switching);assert(!std::strcmp(saved.event.text,"rascunho"));
    finish_switch();assert(contains("#1 Secundario") && !saved.event.text[0]);assert(contains("Texto no secundario"));
    key(KeyAction::Text,'s');key(KeyAction::Text,'2');key(KeyAction::Send);assert(commands.size()==1 && commands[0].channel==1 && commands[0].expected_conversation==56 && !std::strcmp(commands[0].text,"s2"));
    key(KeyAction::Escape);key(KeyAction::Escape);key(KeyAction::Up);key(KeyAction::Enter);assert(!status.switching && contains("Aguarde")); // pending send blocks context loss
    MeshEvent queued;queued.kind=MeshEventKind::Queued;queued.token=commands[0].token;queued.packet_id=10;events.push_back(queued);tick(fake_millis+101);assert(!saved.event.text[0]);
    key(KeyAction::Enter);finish_switch();assert(contains("#0 LongFast") && !std::strcmp(saved.event.text,"rascunho"));
    key(KeyAction::Escape);conversations[1].unread=3;key(KeyAction::Down);assert(contains("Secundario [3]"));ST7789Spi::capture("m5-conversations");
    MeshEvent rx;rx.kind=MeshEventKind::Text;rx.channel=1;rx.destination=0xffffffff;std::strcpy(rx.text,"recebida em outro canal");events.push_back(rx);tick(fake_millis+101);assert(contains("CONVERSAS") && !std::strcmp(saved.event.text,"rascunho"));
    key(KeyAction::Escape);key(KeyAction::Enter);assert(contains("Recebida"));ST7789Spi::capture("m5-message-detail");key(KeyAction::Right);assert(contains("Hora GPS desconhecida"));ST7789Spi::capture("m5-message-data");key(KeyAction::Left);key(KeyAction::Escape);
    key(KeyAction::Tab);key(KeyAction::Tab);assert(saved.view==1);key(KeyAction::Enter);key(KeyAction::Down);assert(contains("SNR --") && contains("Bateria --"));key(KeyAction::Down);assert(contains("Posicao desconhecida"));ST7789Spi::capture("m5-node-position");key(KeyAction::Escape);key(KeyAction::Down);key(KeyAction::Enter);key(KeyAction::Down);assert(contains("Energia externa") && !contains("101%"));
    key(KeyAction::Tab);key(KeyAction::Tab);for(unsigned i=0;i<7;++i)key(KeyAction::Down);key(KeyAction::Enter);assert(contains("UTC") && contains("Fix --"));key(KeyAction::Down);assert(contains("GPS sem fix") && contains("Hora pode estar valida"));ST7789Spi::capture("m5-gps-no-fix");
    snapshot.gps_fix=true;snapshot.latitude=-157000000;snapshot.longitude=-560000000;snapshot.altitude=180;snapshot.gps_fix_millis=fake_millis-1000;tick(fake_millis+101);assert(contains("Lat -15.70000") && contains("Lon -56.00000"));ST7789Spi::capture("m5-gps-fix");
#ifdef CARDPUTER_ADV_M7
    // Return to Nodes and open a DM; the group draft must not be sent to the peer.
    for(unsigned i=0;i<5 && saved.view!=1;++i)key(KeyAction::BackTab);assert(saved.view==1);
    key(KeyAction::Escape);key(KeyAction::Enter);pki_ready=false;key(KeyAction::Enter);assert(!status.switching && contains("DM indisponivel"));pki_ready=true;key(KeyAction::Enter);assert(status.switching && selected_peer==0x1235);
    finish_switch();assert(contains("DM !00001235 PKI") && saved.navigation.chat_peer==0x1235 && !saved.event.text[0]);
    key(KeyAction::Text,'d');key(KeyAction::Send);assert(commands.back().destination==0x1235 && commands.back().expected_conversation==1000+0x1235);
    MeshEvent dmQueued;dmQueued.kind=MeshEventKind::Queued;dmQueued.token=commands.back().token;dmQueued.packet_id=99;events.push_back(dmQueued);tick(fake_millis+101);
    MeshEvent dmRx;dmRx.kind=MeshEventKind::Text;dmRx.node=0x1235;dmRx.destination=snapshot.node;events.push_back(dmRx);tick(fake_millis+101);
    key(KeyAction::Escape);key(KeyAction::Escape);key(KeyAction::Enter);assert(status.switching);finish_switch();assert(!saved.navigation.chat_peer);
#endif
#ifdef CARDPUTER_ADV_M6
    active_slots=3;++channel_revision;tick(fake_millis+101);
    for(unsigned i=0;i<5 && saved.view!=0;++i)key(KeyAction::BackTab);assert(saved.view==0);key(KeyAction::Escape);key(KeyAction::Down);key(KeyAction::Down);assert(contains("Terceiro"));key(KeyAction::Enter);finish_switch();assert(saved.navigation.chat_channel==2 && contains("#2 Terceiro"));key(KeyAction::Text,'t');key(KeyAction::Text,'3');key(KeyAction::Send);assert(commands.back().channel==2 && commands.back().expected_conversation==57);
#endif
}

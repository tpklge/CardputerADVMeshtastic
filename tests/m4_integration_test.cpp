#include "cardputer_adv/m2_console.hpp"
#include "cardputer_adv/app_storage.hpp"
#include "cardputer_adv/mesh_backend.hpp"
#include <ST7789Spi.h>
#include <cassert>
#include <deque>
namespace {
cardputer_adv::StoredRecord saved;
cardputer_adv::StorageStatus status;
cardputer_adv::MeshSnapshot snapshot;
std::deque<cardputer_adv::MeshEvent> events;
std::vector<cardputer_adv::MeshCommand> commands;
unsigned saves=0;
std::array<cardputer_adv::StoredRecord,2> messages;
void tick(std::uint32_t time){fake_millis=time;cardputer_adv::m2_console_tick();}
void key(cardputer_adv::KeyAction action,char ch=0){++fake_millis;cardputer_adv::m2_console_key({action,ch,false});cardputer_adv::m2_console_tick();}
bool contains(const char* value){for(auto& s:ST7789Spi::strings)if(s.find(value)!=std::string::npos)return true;return false;}
}
namespace cardputer_adv {
StorageStatus app_storage_status(){return status;}
bool app_storage_load_ui(StoredRecord& out){out=saved;return true;}
void app_storage_ui(const char* draft,std::uint16_t cursor,std::uint8_t view,std::uint64_t conversation,bool,const UiNavigation* nav){std::strcpy(saved.event.text,draft);saved.cursor=cursor;saved.view=view;saved.conversation=conversation;if(nav)saved.navigation=*nav;++saves;}
bool app_storage_offset(std::int16_t value){saved.utc_offset=value;return true;}
void app_storage_page(std::uint64_t,std::uint64_t){status.busy=true;}
bool app_storage_page_latest(){return true;}
void app_storage_page_newer(std::uint64_t,std::uint64_t identity){app_storage_page(0,identity);}
std::size_t app_storage_page_read(StoredRecord* out,std::size_t capacity){auto count=std::min(capacity,messages.size());for(std::size_t i=0;i<count;++i)out[i]=messages[i];return count;}
std::uint64_t mesh_conversation(std::uint8_t,std::uint32_t){return 55;}
MeshSnapshot mesh_snapshot(){return snapshot;}
bool mesh_submit(const MeshCommand& c){if(!mesh_validate_command(c))return false;commands.push_back(c);return true;}
bool mesh_poll_event(MeshEvent& out){if(events.empty())return false;out=events.front();events.pop_front();return true;}
std::size_t mesh_nodes_page(std::size_t offset,MeshNode* out,std::size_t capacity){if(offset>=2)return 0;auto count=std::min<std::size_t>(2-offset,capacity);for(std::size_t i=0;i<count;++i){out[i]={};out[i].number=0x1234+offset+i;std::strcpy(out[i].short_name,offset+i?"B":"A");}return count;}
std::size_t mesh_channels_page(std::size_t,MeshChannel* out,std::size_t capacity){auto count=std::min<std::size_t>(capacity,8);for(std::size_t i=0;i<count;++i){out[i]={};out[i].index=i;if(!i){out[i].role=1;out[i].encrypted=true;std::strcpy(out[i].name,"LongFast");}}return count;}
}
int main(){
    using namespace cardputer_adv;
    status.ready=true;status.records=2;snapshot.ready=true;snapshot.radio_ready=true;snapshot.storage_ready=true;snapshot.gps_time=true;snapshot.utc=1800000000;snapshot.battery_mv=4050;snapshot.nodes=2;snapshot.region=26;
    std::strcpy(snapshot.channel_name,"LongFast");std::strcpy(snapshot.short_name,"ADV");std::strcpy(snapshot.long_name,"Cardputer");
    saved.kind=StoreKind::Ui;saved.view=3;std::strcpy(saved.event.text,"rascunho");saved.cursor=8;saved.conversation=55;
    messages[0].sequence=2;messages[0].event.kind=MeshEventKind::Text;messages[0].event.node=0x1234;std::strcpy(messages[0].event.text,"Mensagem recebida no ADV");
    messages[1].sequence=1;messages[1].event.kind=MeshEventKind::Queued;std::strcpy(messages[1].event.text,"Mensagem enviada");
    fake_millis=1000;m2_console_begin();m2_console_tick();status.busy=false;tick(1001);tick(1101);assert(contains("Mensagem recebida"));
    assert(ST7789Spi::pixels[120*240+1]);ST7789Spi::capture("chat-menu");
    tick(5999);assert(ST7789Spi::pixels[120*240+1]);auto before=saves;
    MeshEvent rx;rx.kind=MeshEventKind::Text;rx.node=0xabcd;std::strcpy(rx.text,"Outra mensagem");events.push_back(rx);
    tick(6000);assert(!ST7789Spi::pixels[120*240+1]);assert(saves==before && !std::strcmp(saved.event.text,"rascunho"));
    status.busy=false;tick(6001);
    fake_millis=6100;key(KeyAction::Tab);assert(saved.view==3); // opening preserves legacy data before first actual UI save
    assert(ST7789Spi::pixels[120*240+1]);key(KeyAction::Text,'x');assert(saved.view==0 && !std::strcmp(saved.event.text,"rascunhox"));
    tick(11101);assert(!ST7789Spi::pixels[120*240+1]);ST7789Spi::capture("composer-hidden");
    fake_millis=11200;key(KeyAction::Tab);key(KeyAction::Tab);assert(saved.view==1 && !std::strcmp(saved.event.text,"rascunhox"));
    key(KeyAction::Down);assert(saved.navigation.node_selection==1);ST7789Spi::capture("nodes-menu");
    key(KeyAction::Tab);assert(saved.view==2);key(KeyAction::Enter);assert(contains("Slot 0"));ST7789Spi::capture("channels-menu");
    key(KeyAction::Tab);assert(saved.view==3);for(unsigned i=0;i<6;++i)key(KeyAction::Down);key(KeyAction::Enter);key(KeyAction::Backspace);
    assert(!std::strcmp(saved.event.text,"rascunhox"));ST7789Spi::capture("settings-field");
    key(KeyAction::Tab);assert(saved.view==0);key(KeyAction::Text,'y');assert(!std::strcmp(saved.event.text,"rascunhoxy"));
    key(KeyAction::Tab);assert(saved.view==1 && saved.navigation.node_selection==1);
    key(KeyAction::Tab);assert(contains("Slot 0"));key(KeyAction::Tab);key(KeyAction::Enter);assert(saved.utc_offset==-24);
    for(unsigned i=0;i<5;++i)key(KeyAction::Down);key(KeyAction::Enter);assert(contains("Apagar texto"));auto deadline=fake_millis;
    key(KeyAction::Tab);assert(saved.view==3 && contains("Apagar texto"));tick(deadline+5000);assert(contains("Apagar texto"));
    key(KeyAction::Escape);assert(!std::strcmp(saved.event.text,"rascunhoxy"));key(KeyAction::Enter);key(KeyAction::Enter);assert(!saved.event.text[0]);
    key(KeyAction::Tab);key(KeyAction::Tab);assert(saved.view==0);key(KeyAction::Text,'h');key(KeyAction::Text,'i');assert(commands.empty());
    key(KeyAction::Send);assert(commands.size()==1 && !std::strcmp(commands[0].text,"hi"));key(KeyAction::Text,'!');
    MeshEvent queued;queued.kind=MeshEventKind::Queued;queued.token=commands[0].token;queued.packet_id=10;events.push_back(queued);tick(fake_millis+1);
    assert(!std::strcmp(saved.event.text,"hi!")); // a queued callback cannot erase newer edits
}

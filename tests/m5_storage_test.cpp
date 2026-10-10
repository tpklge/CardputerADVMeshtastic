#include "cardputer_adv/app_storage.hpp"
#include <SD.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#ifdef CARDPUTER_ADV_M7_NODES
#include <gps/RTC.h>
#endif
using namespace cardputer_adv;
void steps(unsigned n=1){while(n--)storage_step();}
void settle(){for(unsigned i=0;i<200 && app_storage_status().busy;++i)steps();assert(!app_storage_status().busy);}
void append(std::uint8_t channel,std::uint64_t conversation,std::uint32_t packet,const char* text){MeshEvent e;e.kind=MeshEventKind::Text;e.channel=channel;e.node=0x1234;e.packet_id=packet;std::strcpy(e.text,text);assert(app_storage_event(e,conversation));steps();}
int main(int argc,char** argv){
    namespace fs=std::filesystem;const fs::path fixture=argc>1 && !std::strcmp(argv[1],"--dm-reboot")?".pio/native-dm-fixture":".pio/native-storage-fixture";
    if(argc>1){
        for(auto& f:fs::recursive_directory_iterator(fixture))if(f.is_regular_file()){std::ifstream in(f.path(),std::ios::binary);auto bytes=std::make_shared<std::vector<std::uint8_t>>(std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>());SD.files["/"+fs::relative(f.path(),fixture).generic_string()]=bytes;}
#ifdef CARDPUTER_ADV_M7
        if(!std::strcmp(argv[1],"--dm-reboot")){assert(app_storage_begin());StoredRecord ui;assert(app_storage_load_ui(ui) && ui.navigation.chat_peer==0x1234 && ui.conversation==100 && !std::strcmp(ui.event.text,"private draft"));app_storage_page(0,100);settle();StoredRecord page[4];assert(app_storage_page_read(page,4)==1 && !std::strcmp(page[0].event.text,"private RX"));return 0;}
#endif
        bool interrupted=std::strcmp(argv[1],"--interrupted")==0;
        if(interrupted){StoreWire wire;StoredRecord r;auto bytes=SD.files.at("/CardputerADVMeshtastic/drafts/c0000000000000037-b.bin");std::memcpy(wire.data(),bytes->data(),wire.size());assert(store_decode(wire,r));r.sequence+=100;std::strcpy(r.event.text,"recovered-context");assert(store_encode(r,wire));*bytes=std::vector<std::uint8_t>(wire.begin(),wire.end());}
#ifdef CARDPUTER_ADV_M6
        if(std::strcmp(argv[1],"--url-corrupt")==0){for(auto& f:SD.files)if(f.first.find("/drafts/url-")!=std::string::npos)(*f.second)[100]^=1;assert(!app_storage_begin() && app_storage_status().fault);return 0;}
#endif
        assert(app_storage_begin());StoredRecord ui;assert(app_storage_load_ui(ui));assert(ui.conversation==55 && !std::strcmp(ui.event.text,interrupted?"recovered-context":"draft-primary") && ui.cursor==5);if(interrupted)assert(app_storage_status().recovered);assert(ui.navigation.theme==2);
#ifdef CARDPUTER_ADV_M6
        char restored[2048];std::uint16_t cursor;assert(app_storage_url_load(restored,sizeof(restored),cursor) && cursor==10 && !std::strcmp(restored,"https://meshtastic.org/e/#CgMSAQE"));
#endif
#ifdef CARDPUTER_ADV_M7_NODES
        assert(ui.navigation.node_favorites && !std::strcmp(ui.navigation.node_query,"Alpha"));
        steps(200);auto stamp=app_storage_node_metrics(0x1234);assert(stamp.time && stamp.has_voltage && stamp.voltage==3.9f);
#endif
        std::uint64_t ids[8]{55,56};assert(app_storage_bind_channels(ids,8));steps(20);assert(app_storage_conversation(0).unread==2 && !app_storage_conversation(1).unread);
        assert(app_storage_select(56,1));steps();assert(app_storage_load_ui(ui));assert(!std::strcmp(ui.event.text,"Ol\xc3\xa1 no secundario") && ui.navigation.read_sequence==app_storage_conversation(1).latest);return 0;
    }
    // Boot with an M4 UI record already on the card.
    StoredRecord legacy;legacy.kind=StoreKind::Ui;legacy.sequence=4;legacy.conversation=55;legacy.navigation.shell=true;std::strcpy(legacy.event.text,"draft-primary");legacy.cursor=5;
    StoreWire legacy_wire;assert(store_encode(legacy,legacy_wire));SD.files["/CardputerADVMeshtastic/drafts/a.bin"]=std::make_shared<std::vector<std::uint8_t>>(legacy_wire.begin(),legacy_wire.end());
    assert(app_storage_begin());StoredRecord boot_ui;assert(app_storage_load_ui(boot_ui) && !boot_ui.navigation.conversations && !std::strcmp(boot_ui.event.text,"draft-primary"));UiNavigation nav;nav.shell=true;nav.conversations=true;nav.chat_channel=0;nav.theme=2;
    app_storage_ui("draft-primary",5,0,55,true,&nav);steps();
    // Continuous edits must produce durable drafts before the idle debounce.
    auto persisted=[&](){std::uint64_t generation=0;for(auto& f:SD.files)if(f.first.find("/drafts/")!=std::string::npos && f.second->size()==kStoreRecordBytes){StoreWire w;std::memcpy(w.data(),f.second->data(),w.size());StoredRecord r;if(store_decode(w,r))generation=std::max(generation,r.sequence);}return generation;};
    auto generation=persisted();char ongoing[32];
    for(unsigned i=0;i<55;++i){std::snprintf(ongoing,sizeof(ongoing),"typing-%u",i);app_storage_ui(ongoing,0,0,55,false,&nav);fake_storage_millis+=100;steps();}
    assert(persisted()>generation);
    app_storage_ui("draft-primary",5,0,55,true,&nav);steps();generation=persisted();
    for(unsigned i=0;i<20;++i){app_storage_ui("draft-primary",5,0,55,true,&nav);steps();}assert(persisted()==generation);
    append(0,55,1,"primary-one");append(1,56,2,"secondary-one");append(0,55,3,"primary-two");append(1,56,4,"secondary-two");
    std::uint64_t ids[8]{55,56};assert(app_storage_bind_channels(ids,8));steps(20);
    assert(app_storage_conversation(0).unread==2 && app_storage_conversation(1).unread==2);
    app_storage_page(0,56);settle();StoredRecord page[4];auto count=app_storage_page_read(page,4);assert(count==2 && page[0].conversation==56 && !std::strcmp(page[0].event.text,"secondary-two"));
    app_storage_mark_read(56,page[0].sequence);steps();assert(!app_storage_conversation(1).unread);
    // The primary draft must be durable before a switch completes.
    assert(app_storage_select(56,1));assert(app_storage_status().switching);steps();StoredRecord ui;assert(app_storage_load_ui(ui));assert(ui.conversation==56 && !ui.event.text[0]);
    nav=ui.navigation;app_storage_ui("draft-secondary",7,0,56,true,&nav);steps();
    assert(app_storage_select(55,0));steps();assert(app_storage_load_ui(ui));assert(!std::strcmp(ui.event.text,"draft-primary") && ui.cursor==5 && ui.navigation.chat_channel==0 && ui.navigation.theme==2);
    assert(app_storage_select(56,1));steps();assert(app_storage_load_ui(ui));assert(!std::strcmp(ui.event.text,"draft-secondary") && ui.cursor==7);
    assert(SD.files.count("/CardputerADVMeshtastic/drafts/c0000000000000037-a.bin") && SD.files.count("/CardputerADVMeshtastic/drafts/c0000000000000037-b.bin"));
    // Per-channel read watermark is persisted even with a different channel open.
    app_storage_mark_read(56,app_storage_conversation(1).latest);steps();assert(app_storage_bind_channels(ids,8));steps(20);assert(!app_storage_conversation(1).unread && app_storage_conversation(0).unread==2);
    // Corrupt one context generation: recover the other, keeping full UTF-8 text.
    nav=ui.navigation;app_storage_ui("Ol\xc3\xa1 no secundario",3,0,56,true,&nav);steps();app_storage_ui("Ol\xc3\xa1 no secundario",4,0,56,true,&nav);steps();
    assert(app_storage_select(55,0));steps();
#ifdef CARDPUTER_ADV_M6
    char long_link[2048];std::memset(long_link,'a',2047);long_link[2047]=0;assert(app_storage_url(long_link,2047));steps(1001);char restored[2048];std::uint16_t cursor;assert(app_storage_url_load(restored,sizeof(restored),cursor) && cursor==2047 && !std::strcmp(restored,long_link));
    assert(app_storage_url("https://meshtastic.org/e/#CgMSAQE",10,true));steps();assert(app_storage_url("https://meshtastic.org/e/#CgMSAQE",10));steps(1001);bool exported=false;for(auto& f:SD.files)if(f.first.find("/exports/")!=std::string::npos){exported=true;assert(std::string(f.second->begin(),f.second->end())=="https://meshtastic.org/e/#CgMSAQE");}assert(exported);
#endif
#ifdef CARDPUTER_ADV_M7
    assert(!app_storage_select_peer(100,0,1));
    assert(app_storage_select_peer(100,0,0x1234));steps();assert(app_storage_load_ui(ui) && ui.navigation.chat_peer==0x1234 && !ui.event.text[0]);
    nav=ui.navigation;app_storage_ui("private draft",7,0,100,true,&nav);steps();
    assert(app_storage_select(55,0));steps();assert(app_storage_load_ui(ui) && !ui.navigation.chat_peer && !std::strcmp(ui.event.text,"draft-primary"));
    assert(app_storage_select_peer(100,0,0x1234));steps();assert(app_storage_load_ui(ui) && ui.navigation.chat_peer==0x1234 && !std::strcmp(ui.event.text,"private draft"));
    append(0,100,45,"private RX");
    fs::remove_all(".pio/native-dm-fixture");for(auto& f:SD.files){auto path=fs::path(".pio/native-dm-fixture")/f.first.substr(1);fs::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(f.second->data()),f.second->size());}
    app_storage_page(0,100);settle();assert(app_storage_page_read(page,4)==1 && page[0].conversation==100);
    MeshEvent queued;queued.kind=MeshEventKind::Queued;queued.packet_id=99;queued.destination=0x1234;queued.node=2;std::strcpy(queued.text,"private sent");assert(app_storage_event(queued,100));steps();
    MeshEvent feedback;feedback.kind=MeshEventKind::RoutingFeedback;feedback.packet_id=99;assert(app_storage_event(feedback,0));steps();
    app_storage_page(0,100);settle();assert(app_storage_page_read(page,4)==2 && page[0].delivery==MeshEventKind::RoutingFeedback && page[0].event.result==0);
    feedback.result=3;assert(app_storage_event(feedback,0));steps();app_storage_page(0,100);settle();assert(app_storage_page_read(page,4)==2 && page[0].event.result==3);
    assert(app_storage_select(55,0));steps();
#endif
#ifdef CARDPUTER_ADV_M7_NODES
    assert(app_storage_load_ui(ui));nav=ui.navigation;nav.node_favorites=true;std::strcpy(nav.node_query,"Alpha");app_storage_ui(ui.event.text,ui.cursor,ui.view,ui.conversation,true,&nav);steps();
    MeshEvent metrics;metrics.kind=MeshEventKind::Telemetry;metrics.node=0x1234;metrics.has_voltage=true;metrics.voltage=3.9f;assert(app_storage_event(metrics,0));steps();auto stamp=app_storage_node_metrics(0x1234);assert(stamp.node==0x1234 && stamp.time && stamp.has_voltage && stamp.voltage==3.9f);
    fake_storage_utc=0;metrics.node=0x5678;assert(app_storage_event(metrics,0));steps();assert(app_storage_node_metrics(0x5678).time==0);fake_storage_utc=1800000000;
#endif
    // Paging forward returns the nearest newer messages, not an immediate jump.
    for(unsigned i=1;i<=10;++i){char body[24];std::snprintf(body,sizeof(body),"history-%u",i);append(0,999,1000+i,body);}
    app_storage_page(0,999);settle();assert(app_storage_page_read(page,4)==4 && !std::strcmp(page[0].event.text,"history-10"));auto older=page[3].sequence;
    app_storage_page(older,999);settle();assert(app_storage_page_read(page,4)==4 && !std::strcmp(page[0].event.text,"history-6"));auto last=page[0].sequence;
    app_storage_page_newer(last,999);settle();assert(app_storage_page_read(page,4)==4 && !std::strcmp(page[0].event.text,"history-10") && app_storage_page_latest());
    app_storage_page(older,999);settle();app_storage_page_read(page,4);auto first=page[3].sequence;
    app_storage_page_newer(first,999);settle();assert(app_storage_page_read(page,4)==4 && !std::strcmp(page[3].event.text,"history-4") && !app_storage_page_latest());
    // Save a card image for a fresh-process reboot test before corruption injection.
    fs::remove_all(fixture);
    for(auto& f:SD.files){auto path=fixture/f.first.substr(1);fs::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(f.second->data()),f.second->size());}
    auto a=SD.files.find("/CardputerADVMeshtastic/drafts/c0000000000000038-a.bin");assert(a!=SD.files.end());(*a->second)[110]^=1;
    assert(app_storage_select(56,1));steps();assert(app_storage_load_ui(ui));assert(!std::strcmp(ui.event.text,"Ol\xc3\xa1 no secundario"));assert(app_storage_status().recovered && !app_storage_status().fault);
    // Lose both generations of a non-current context: preserve files, report SD fault.
    assert(app_storage_select(55,0));steps();
    for(auto& f:SD.files)if(f.first.find("/drafts/c0000000000000038-")!=std::string::npos)(*f.second)[110]^=1;
    // Ensure both are invalid regardless of which was repaired by conservative archival.
    for(auto& f:SD.files)if(f.first.find("/drafts/c0000000000000038-")!=std::string::npos){StoreWire wire;std::memcpy(wire.data(),f.second->data(),wire.size());StoredRecord r;if(store_decode(wire,r))(*f.second)[111]^=1;}
    assert(app_storage_select(56,1));steps();assert(app_storage_status().fault && !app_storage_status().switching);assert(SD.files.count("/CardputerADVMeshtastic/drafts/c0000000000000038-a.bin"));
}

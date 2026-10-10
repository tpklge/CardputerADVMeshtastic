#ifdef CARDPUTER_ADV_M3
#include "cardputer_adv/app_storage.hpp"
#include "cardputer_adv/core_storage.hpp"
#include "cardputer_adv/shared_spi.hpp"
#include <Arduino.h>
#include <SD.h>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <esp_system.h>
#include "gps/RTC.h"

namespace cardputer_adv {
namespace {
constexpr unsigned kSegments=96,kPerSegment=512,kPage=4;
constexpr std::size_t kHeader=32;
constexpr const char* base="/CardputerADVMeshtastic";
struct Segment {std::uint64_t generation=0,first=0;unsigned records=0,slot=0;bool partial=false;};
std::array<Segment,kSegments> segments{};
unsigned segment_count=0;
Segment active;
struct Job {StoredRecord record;bool synchronous=false;};
BoundedQueue<Job,16> queue;
SemaphoreHandle_t sync_done=nullptr,sync_mutex=nullptr;
bool sync_result=false;
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
StorageStatus status;
std::uint64_t boot=0,next_sequence=1,ui_generation=0;
StoredRecord ui;
bool ui_dirty=false,force_ui=false,ui_loaded=false,ui_writing=false;
std::uint32_t ui_at=0,ui_dirty_since=0;
#ifdef CARDPUTER_ADV_M6
char url_draft[2048]{};std::uint16_t url_cursor=0;std::uint64_t url_generation=0;bool url_dirty=false,url_export=false;std::uint32_t url_at=0,url_dirty_since=0;
std::array<std::uint8_t,2080> url_wire{};
bool url_io(bool writing,const String& path){File f;{SharedSpiGuard g;f=SD.open(path,writing?FILE_WRITE:FILE_READ);if(!f || (!writing && f.size()!=url_wire.size())){f.close();return false;}}for(unsigned at=0;at<url_wire.size();at+=256){SharedSpiGuard g;auto count=std::min<std::size_t>(256,url_wire.size()-at);if((writing?f.write(url_wire.data()+at,count):f.read(url_wire.data()+at,count))!=count){f.close();return false;}}{SharedSpiGuard g;if(writing)f.flush();f.close();}return true;}
#endif
std::array<StoredRecord,kPage> page{};
std::size_t page_count=0;
bool page_requested=false,page_after=false,page_latest=true;
std::uint64_t page_before=0,page_conversation=0;
#ifdef CARDPUTER_ADV_M5
std::array<ConversationStatus,8> conversations{};
std::array<std::uint64_t,8> requested_channels{};
bool channels_requested=false;
std::uint64_t switch_conversation=0,index_upper=0;
std::uint8_t switch_channel=0;
std::uint32_t switch_peer=0;
int index_segment=-1,index_record=-1;
#endif
int page_segment=-1,page_record=-1;
int verify_segment=0;
unsigned verify_record=0;
bool scanning=false;
bool finding_states=false;
#ifdef CARDPUTER_ADV_M7_NODES
std::array<NodeMetricsStamp,200> metrics_stamps{};
void remember_metrics(const StoredRecord& r){if(r.kind!=StoreKind::Event || r.event.kind!=MeshEventKind::Telemetry || !r.event.node)return;portENTER_CRITICAL(&mux);NodeMetricsStamp* target=nullptr;for(auto& s:metrics_stamps)if(s.node==r.event.node){target=&s;break;}if(!target)for(auto& s:metrics_stamps)if(!s.node){target=&s;break;}if(!target)target=&*std::min_element(metrics_stamps.begin(),metrics_stamps.end(),[](const NodeMetricsStamp& a,const NodeMetricsStamp& b){return a.sequence<b.sequence;});if(target->node!=r.event.node || r.sequence>target->sequence){*target={};target->node=r.event.node;target->sequence=r.sequence;target->time=r.utc_valid?r.utc:0;target->voltage=r.event.voltage;target->battery=r.event.battery;target->has_voltage=r.event.has_voltage;target->has_battery=r.event.has_battery;}portEXIT_CRITICAL(&mux);}
#endif
std::array<RxFingerprint,32> recent_rx{};
unsigned recent_rx_at=0;
void fail() {portENTER_CRITICAL(&mux);status.fault=true;status.ready=false;status.switching=false;portEXIT_CRITICAL(&mux);}
String log_path(unsigned slot) {char name[80];std::snprintf(name,sizeof(name),"%s/messages/s%02u.log",base,slot);return String(name);}
String ui_path(unsigned slot) {return String(base)+"/drafts/"+(slot?"b":"a")+".bin";}
void put(std::uint8_t* p,std::uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)p[i]=v>>(8*i);}
std::uint64_t get(const std::uint8_t* p,unsigned n){std::uint64_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint64_t(p[i])<<(8*i);return v;}
bool read_wire(const String& path,std::size_t offset,StoreWire& wire) {
    SharedSpiGuard g;auto f=SD.open(path,FILE_READ);return f && f.seek(offset) && f.read(wire.data(),wire.size())==wire.size();
}
#ifdef CARDPUTER_ADV_M5
String context_path(std::uint64_t conversation,unsigned slot) {char name[100];std::snprintf(name,sizeof(name),"%s/drafts/c%016llx-%c.bin",base,static_cast<unsigned long long>(conversation),slot?'b':'a');return String(name);}
bool load_context(std::uint64_t conversation,StoredRecord& out,unsigned* latest_slot=nullptr) {
    bool loaded=false,damaged=false;
    for(unsigned slot=0;slot<2;++slot){StoreWire wire;StoredRecord r;
        bool exists;{SharedSpiGuard g;exists=SD.exists(context_path(conversation,slot));}
        if(!exists)continue;
        if(read_wire(context_path(conversation,slot),0,wire)) {
            if(!std::memcmp(wire.data(),"ADV3",4) && (wire[4]>1 || wire[5])){fail();return false;}
            if(store_decode(wire,r) && r.kind==StoreKind::Ui && r.conversation==conversation){if(!loaded || r.sequence>out.sequence){out=r;if(latest_slot)*latest_slot=slot;}loaded=true;}else damaged=true;
        }else damaged=true;
    }
    if(damaged){portENTER_CRITICAL(&mux);status.recovered=true;portEXIT_CRITICAL(&mux);if(!loaded)fail();}
    return loaded;
}
#endif
#ifdef CARDPUTER_ADV_M6
bool load_url(){bool seen=false,valid=false;for(unsigned slot=0;slot<2;++slot){auto path=String(base)+"/drafts/url-"+(slot?"b":"a")+".bin";bool exists;{SharedSpiGuard g;exists=SD.exists(path);}if(!exists)continue;seen=true;if(!url_io(false,path) || std::memcmp(url_wire.data(),"URL6",4) || get(url_wire.data()+2076,4)!=store_crc(url_wire.data(),2076)){status.recovered=true;continue;}if(url_wire[4]!=1){fail();return false;}auto len=get(url_wire.data()+16,2),cursor=get(url_wire.data()+18,2),generation=get(url_wire.data()+8,8);if(len>2047 || cursor>len || (cursor<len && (url_wire[20+cursor]&0xc0)==0x80) || url_wire[20+len] || !mesh_valid_utf8(reinterpret_cast<char*>(url_wire.data()+20),len+1,true)){fail();return false;}valid=true;if(generation>url_generation){url_generation=generation;std::memcpy(url_draft,url_wire.data()+20,len+1);url_cursor=cursor;}}if(seen && !valid){fail();return false;}return true;}
bool save_url(){if(url_generation==UINT64_MAX)return false;portENTER_CRITICAL(&mux);url_wire.fill(0);std::memcpy(url_wire.data()+20,url_draft,2048);auto cursor=url_cursor;bool export_file=url_export;url_dirty=false;url_export=false;portEXIT_CRITICAL(&mux);std::memcpy(url_wire.data(),"URL6",4);url_wire[4]=1;put(url_wire.data()+8,++url_generation,8);auto len=std::strlen(reinterpret_cast<char*>(url_wire.data()+20));put(url_wire.data()+16,len,2);put(url_wire.data()+18,cursor,2);put(url_wire.data()+2076,store_crc(url_wire.data(),2076),4);auto crc=get(url_wire.data()+2076,4);auto path=String(base)+"/drafts/url-"+(url_generation%2?"b":"a")+".bin";if(!url_io(true,path) || !url_io(false,path) || get(url_wire.data()+2076,4)!=crc || store_crc(url_wire.data(),2076)!=crc)return false;
 if(export_file){String dir=String(base)+"/exports";{SharedSpiGuard g;if(!SD.exists(dir) && !SD.mkdir(dir))return false;}char name[100];std::snprintf(name,sizeof(name),"%s/channels-%llu.url",dir.c_str(),static_cast<unsigned long long>(url_generation));File f;{SharedSpiGuard g;f=SD.open(name,FILE_WRITE);}if(!f)return false;for(unsigned at=0;at<len;at+=256){SharedSpiGuard g;auto n=std::min<std::size_t>(256,len-at);if(f.write(url_wire.data()+20+at,n)!=n){f.close();return false;}}{SharedSpiGuard g;f.flush();f.close();}{SharedSpiGuard g;f=SD.open(name,FILE_READ);if(!f || f.size()!=len){f.close();return false;}}
    std::uint8_t check[256];for(unsigned at=0;at<len;at+=256){SharedSpiGuard g;auto n=std::min<std::size_t>(256,len-at);if(f.read(check,n)!=n || std::memcmp(check,url_wire.data()+20+at,n)){f.close();return false;}}{SharedSpiGuard g;f.close();}portENTER_CRITICAL(&mux);++status.exports;portEXIT_CRITICAL(&mux);
 }
 return true;}
#endif
bool read_record(const Segment& s,unsigned index,StoredRecord& record) {
    StoreWire wire;
    if(!read_wire(log_path(s.slot),kHeader+index*kStoreRecordBytes,wire))return false;
    if(!std::memcmp(wire.data(),"ADV3",4) && (wire[4]>1 || wire[5])) {fail();return false;}
    return store_decode(wire,record) && record.sequence==s.first+index;
}
bool create_segment() {
    if(active.generation==UINT64_MAX)return false;
    auto generation=active.generation+1;
    unsigned slot=(generation-1)%kSegments;
    auto path=log_path(slot);
    {SharedSpiGuard g;
        if(SD.exists(path)) {
            // Retention only retires the oldest slot, never the active segment.
            if(slot==active.slot && active.generation)return false;
            if(!SD.remove(path))return false;
        }
        std::uint8_t header[kHeader]{};std::memcpy(header,"ADL3",4);header[4]=1;put(header+8,generation,8);put(header+16,next_sequence,8);put(header+28,store_crc(header,28),4);
        auto f=SD.open(path,FILE_WRITE);if(!f || f.write(header,kHeader)!=kHeader)return false;f.flush();f.close();
        std::uint8_t check[kHeader];f=SD.open(path,FILE_READ);if(!f || f.read(check,kHeader)!=kHeader || std::memcmp(check,header,kHeader))return false;
    }
    active={generation,next_sequence,0,slot};
    for(unsigned index=0;index<segment_count;++index)if(segments[index].slot==slot) {
        auto retired=segments[index].records;
        if(page_segment>=int(index))--page_segment;
#ifdef CARDPUTER_ADV_M5
        if(index_segment>=int(index))--index_segment;
        portENTER_CRITICAL(&mux);channels_requested=true;portEXIT_CRITICAL(&mux);
#endif
        if(verify_segment>int(index))--verify_segment;
        std::move(segments.begin()+index+1,segments.begin()+segment_count,segments.begin()+index);--segment_count;
        portENTER_CRITICAL(&mux);status.retired+=retired;status.records-=std::min<std::uint64_t>(status.records,retired);portEXIT_CRITICAL(&mux);
        break;
    }
    segments[segment_count++]=active;return true;
}
bool append(StoredRecord r) {
    if(r.kind==StoreKind::Event && r.event.kind==MeshEventKind::Text) {
        auto incoming=store_fingerprint(r);
        for(auto& previous:recent_rx)if(store_same_rx(previous,incoming))return true;
    }
    if(!active.generation || active.records==kPerSegment)if(!create_segment())return false;
    r.sequence=next_sequence;StoreWire wire;if(!store_encode(r,wire))return false;
    auto path=log_path(active.slot);
    {SharedSpiGuard g;auto f=SD.open(path,FILE_APPEND);if(!f || f.size()!=kHeader+active.records*kStoreRecordBytes || f.write(wire.data(),wire.size())!=wire.size())return false;f.flush();f.close();}
    StoreWire verified;if(!read_wire(path,kHeader+active.records*kStoreRecordBytes,verified) || verified!=wire)return false;
#ifdef CARDPUTER_ADV_M7_NODES
    remember_metrics(r);
#endif
    ++active.records;++next_sequence;segments[segment_count-1]=active;
    if(r.kind==StoreKind::Event && r.event.kind==MeshEventKind::Text)recent_rx[recent_rx_at++%recent_rx.size()]=store_fingerprint(r);
    portENTER_CRITICAL(&mux);++status.records;
#ifdef CARDPUTER_ADV_M5
    for(auto& c:conversations)if(c.identity && c.identity==r.conversation && (r.event.kind==MeshEventKind::Text || r.event.kind==MeshEventKind::Queued)) {c.latest=std::max(c.latest,r.sequence);if(r.event.kind==MeshEventKind::Text && r.sequence>c.read && c.unread<UINT32_MAX)++c.unread;}
#endif
    portEXIT_CRITICAL(&mux);return true;
}
bool save_ui(StoredRecord record) {
    // Preserve a draft belonging to a different logical channel before reusing UI slots.
    for(unsigned slot=0;slot<2;++slot) {
        StoreWire old;StoredRecord previous;
        if(read_wire(ui_path(slot),0,old) && store_decode(old,previous) && previous.kind==StoreKind::Ui && previous.conversation!=record.conversation && previous.event.text[0]) {
#ifdef CARDPUTER_ADV_M5
            if(record.navigation.conversations){
                StoredRecord existing;unsigned latest=1;bool loaded=load_context(previous.conversation,existing,&latest);portENTER_CRITICAL(&mux);bool fault=status.fault;portEXIT_CRITICAL(&mux);if(fault)return false;
                if(!loaded || existing.sequence<previous.sequence){auto context=context_path(previous.conversation,loaded?1-latest:0);
                    {SharedSpiGuard g;auto f=SD.open(context,FILE_WRITE);if(!f || f.write(old.data(),old.size())!=old.size())return false;f.flush();f.close();}
                    StoreWire check;if(!read_wire(context,0,check) || check!=old)return false;
                }
                continue;
            }
#endif
            char name[112];std::snprintf(name,sizeof(name),"%s/drafts/context-%016llx-%llu.bin",base,static_cast<unsigned long long>(previous.conversation),static_cast<unsigned long long>(previous.sequence));
            SharedSpiGuard g;if(!SD.exists(name)) {auto f=SD.open(name,FILE_WRITE);if(!f || f.write(old.data(),old.size())!=old.size())return false;f.flush();f.close();}
        }
    }
#ifdef CARDPUTER_ADV_M5
    unsigned context_slot=0;
    if(record.navigation.conversations && record.conversation){StoredRecord existing;unsigned latest=1;bool loaded=load_context(record.conversation,existing,&latest);
        portENTER_CRITICAL(&mux);bool fault=status.fault;portEXIT_CRITICAL(&mux);if(fault)return false;
        if(loaded){context_slot=1-latest;ui_generation=std::max(ui_generation,existing.sequence);}
    }
#endif
    record.kind=StoreKind::Ui;record.sequence=++ui_generation;record.boot=boot;
    StoreWire wire;if(!store_encode(record,wire))return false;
#ifdef CARDPUTER_ADV_M5
    if(record.navigation.conversations && record.conversation) {
        auto context=context_path(record.conversation,context_slot);
        {SharedSpiGuard g;auto f=SD.open(context,FILE_WRITE);if(!f || f.write(wire.data(),wire.size())!=wire.size())return false;f.flush();f.close();}
        StoreWire check;if(!read_wire(context,0,check) || check!=wire)return false;
    }
#endif
    auto path=ui_path(ui_generation%2);
    {SharedSpiGuard g;auto f=SD.open(path,FILE_WRITE);if(!f || f.write(wire.data(),wire.size())!=wire.size())return false;f.flush();f.close();}
    StoreWire check;return read_wire(path,0,check) && check==wire;
}
bool has_fault(){portENTER_CRITICAL(&mux);bool value=status.fault;portEXIT_CRITICAL(&mux);return value;}
#ifdef CARDPUTER_ADV_M5
void switch_context() {
    StoredRecord old;std::uint64_t identity;std::uint8_t channel;
    portENTER_CRITICAL(&mux);old=ui;identity=switch_conversation;channel=switch_channel;ui_dirty=false;force_ui=false;portEXIT_CRITICAL(&mux);
    if(!save_ui(old)){fail();return;}
    StoredRecord next;bool loaded=load_context(identity,next);
    if(has_fault())return;
    if(loaded)ui_generation=std::max(ui_generation,next.sequence);
    auto navigation=old.navigation;
    if(loaded){navigation.history_before=next.navigation.history_before;navigation.history_selection=next.navigation.history_selection;navigation.history_first=next.navigation.history_first;navigation.detail_sequence=next.navigation.detail_sequence;navigation.detail_row=next.navigation.detail_row;navigation.read_sequence=next.navigation.read_sequence;}
    else {navigation.history_before=0;navigation.history_selection=0;navigation.history_first=0;navigation.detail_sequence=0;navigation.detail_row=0;navigation.read_sequence=0;}
    navigation.conversations=true;navigation.chat_channel=channel;navigation.chat_peer=switch_peer;next.navigation=navigation;next.utc_offset=old.utc_offset;next.view=old.view;next.conversation=identity;next.kind=StoreKind::Ui;
    if(!save_ui(next)){fail();return;}
    portENTER_CRITICAL(&mux);ui=next;ui_loaded=true;status.switching=false;portEXIT_CRITICAL(&mux);
}
void bind_contexts() {
    std::array<std::uint64_t,8> ids;StoredRecord current;
    portENTER_CRITICAL(&mux);ids=requested_channels;channels_requested=false;current=ui;portEXIT_CRITICAL(&mux);
    std::array<ConversationStatus,8> next{};
    for(unsigned i=0;i<8;++i)if(ids[i]) {auto& c=next[i];c.identity=ids[i];c.scanning=true;StoredRecord saved;
        if(load_context(c.identity,saved))c.read=saved.navigation.read_sequence;
        if(current.conversation==c.identity)c.read=std::max(c.read,current.navigation.read_sequence);
    }
    index_upper=next_sequence;index_segment=int(segment_count)-1;index_record=index_segment>=0?int(segments[index_segment].records)-1:-1;
    portENTER_CRITICAL(&mux);conversations=next;portEXIT_CRITICAL(&mux);
}
void scan_conversations() {
    for(unsigned batch=0;batch<8 && index_segment>=0;++batch) {
        if(index_record<0){--index_segment;if(index_segment>=0)index_record=int(segments[index_segment].records)-1;continue;}
        StoredRecord r;if(!read_record(segments[index_segment],index_record--,r) || r.sequence>=index_upper || r.kind!=StoreKind::Event)continue;
        if(r.event.kind!=MeshEventKind::Text && r.event.kind!=MeshEventKind::Queued)continue;
        portENTER_CRITICAL(&mux);
        for(auto& c:conversations)if(c.identity && c.identity==r.conversation){c.latest=std::max(c.latest,r.sequence);if(r.event.kind==MeshEventKind::Text && r.sequence>c.read && c.unread<UINT32_MAX)++c.unread;}
        portEXIT_CRITICAL(&mux);
    }
    if(index_segment<0){portENTER_CRITICAL(&mux);for(auto& c:conversations)c.scanning=false;portEXIT_CRITICAL(&mux);}
}
#endif
void worker(void*) {
    while(true) {
        if(!has_fault()) {
            Job job;portENTER_CRITICAL(&mux);bool has=queue.pop(job);portEXIT_CRITICAL(&mux);
            if(has) {
                bool ok=append(job.record);if(!ok)fail();
                if(job.synchronous) {portENTER_CRITICAL(&mux);sync_result=ok;portEXIT_CRITICAL(&mux);xSemaphoreGive(sync_done);}
            }
#ifdef CARDPUTER_ADV_M5
            portENTER_CRITICAL(&mux);bool switching=status.switching,bind=channels_requested;portEXIT_CRITICAL(&mux);
            if(switching)switch_context();
            if(bind && !has_fault())bind_contexts();
            if(!has_fault())scan_conversations();
#endif
#ifdef CARDPUTER_ADV_M6
            portENTER_CRITICAL(&mux);bool save_link=url_dirty && (url_export || std::uint32_t(millis()-url_at)>=1000 || std::uint32_t(millis()-url_dirty_since)>=5000);portEXIT_CRITICAL(&mux);if(save_link && !save_url())fail();
#endif
            bool save=false;StoredRecord copy;
            portENTER_CRITICAL(&mux);
            if(ui_dirty && (force_ui || std::uint32_t(millis()-ui_at)>=1000 || std::uint32_t(millis()-ui_dirty_since)>=5000)) {copy=ui;ui_dirty=false;force_ui=false;save=true;ui_writing=true;}
            portEXIT_CRITICAL(&mux);
            if(save){if(!save_ui(copy))fail();portENTER_CRITICAL(&mux);ui_writing=false;portEXIT_CRITICAL(&mux);}
            portENTER_CRITICAL(&mux);
            if(page_requested) {page_count=0;finding_states=false;page_segment=int(segment_count)-1;page_record=page_segment>=0?int(segments[page_segment].records)-1:-1;page_requested=false;page_latest=!page_before || page_after;status.busy=true;}
            bool busy=status.busy;auto before=page_before,conversation=page_conversation;bool after=page_after;
            portEXIT_CRITICAL(&mux);
            if(busy)for(unsigned batch=0;batch<8;++batch) {
                if(page_segment<0) {
                    if(!finding_states && page_count) {finding_states=true;page_segment=int(segment_count)-1;page_record=segments[page_segment].records-1;}
                    else {portENTER_CRITICAL(&mux);status.busy=false;portEXIT_CRITICAL(&mux);break;}
                }
                if(page_record<0) {--page_segment;if(page_segment>=0)page_record=segments[page_segment].records-1;continue;}
                StoredRecord found;
                if(read_record(segments[page_segment],page_record--,found)) {
                    if(finding_states) {
                        if(found.kind==StoreKind::Event && (found.event.kind==MeshEventKind::Transmitted || found.event.kind==MeshEventKind::RoutingFeedback || found.event.kind==MeshEventKind::Error))for(std::size_t i=0;i<page_count;++i) {
                            auto& queued=page[i];
                            if(queued.event.kind==MeshEventKind::Queued && queued.delivery==MeshEventKind::Queued && queued.boot==found.boot &&
                               ((found.event.packet_id && found.event.packet_id==queued.event.packet_id) || (!found.event.packet_id && found.event.token && found.event.token==queued.event.token))) {
                                queued.delivery=found.event.kind;queued.event.result=found.event.result;
                            }
                        }
                        if(found.sequence<=page[page_count-1].sequence) {portENTER_CRITICAL(&mux);status.busy=false;portEXIT_CRITICAL(&mux);break;}
                    } else if(after && found.sequence<=before){page_segment=-1;continue;}
                    else if(found.kind==StoreKind::Event && (found.event.kind==MeshEventKind::Text || found.event.kind==MeshEventKind::Queued) && (after || !before || found.sequence<before) && (!conversation || found.conversation==conversation)) {
                        if(after && page_count==kPage){std::move(page.begin()+1,page.end(),page.begin());page.back()=found;portENTER_CRITICAL(&mux);page_latest=false;portEXIT_CRITICAL(&mux);}
                        else page[page_count++]=found;
                        if(!after && page_count==kPage) {finding_states=true;page_segment=int(segment_count)-1;page_record=segments[page_segment].records-1;break;}
                    }
                } else {portENTER_CRITICAL(&mux);++status.corrupt;status.recovered=true;portEXIT_CRITICAL(&mux);}
            }
            // Validate older records incrementally without blocking boot or radio.
            if(scanning)for(unsigned batch=0;batch<4;++batch) {
                if(verify_segment>=int(segment_count)) {scanning=false;break;}
                const auto s=segments[verify_segment];
                if(verify_record>=s.records) {++verify_segment;verify_record=0;continue;}
                StoredRecord checked;if(!read_record(s,verify_record++,checked)) {portENTER_CRITICAL(&mux);++status.corrupt;status.recovered=true;portEXIT_CRITICAL(&mux);}
#ifdef CARDPUTER_ADV_M7_NODES
                else remember_metrics(checked);
#endif
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
}
bool app_storage_begin() {
    if(core_storage_fault())return false;
    status.recovered=core_storage_recovered();
    {
        SharedSpiGuard g;auto path=String(base)+"/schema.bin";
        std::uint8_t schema[12]{'A','D','P','3',1,0,0,0};put(schema+8,store_crc(schema,8),4);
        if(SD.exists(path)) {auto f=SD.open(path,FILE_READ);std::uint8_t saved[12];if(!f || f.size()!=12 || f.read(saved,12)!=12 || std::memcmp(saved,schema,12)){fail();return false;}}
        else {auto f=SD.open(path,FILE_WRITE);if(!f || f.write(schema,12)!=12){fail();return false;}f.flush();}
    }
    sync_done=xSemaphoreCreateBinary();sync_mutex=xSemaphoreCreateMutex();
    if(!sync_done || !sync_mutex){fail();return false;}
    for(const char* name:{"messages","drafts","indexes"}) {SharedSpiGuard g;auto path=String(base)+"/"+name;if(!SD.exists(path) && !SD.mkdir(path)){fail();return false;}}
    for(unsigned slot=0;slot<2;++slot) {StoreWire wire;if(read_wire(ui_path(slot),0,wire) && !std::memcmp(wire.data(),"ADV3",4) && (wire[4]>1 || wire[5])){fail();return false;}}
    for(unsigned slot=0;slot<kSegments;++slot) {
        SharedSpiGuard g;auto f=SD.open(log_path(slot),FILE_READ);if(!f)continue;
        std::uint8_t header[kHeader];if(f.size()<kHeader || f.read(header,kHeader)!=kHeader || std::memcmp(header,"ADL3",4) || header[4]!=1 || get(header+28,4)!=store_crc(header,28)) {fail();return false;}
        auto generation=get(header+8,8),first=get(header+16,8);
        auto bytes=f.size()-kHeader;
        if(!generation || !first || bytes/kStoreRecordBytes>kPerSegment || (generation-1)%kSegments!=slot){fail();return false;}
        segments[segment_count++]={generation,first,unsigned(bytes/kStoreRecordBytes),slot,bool(bytes%kStoreRecordBytes)};
        if(bytes%kStoreRecordBytes){status.recovered=true;++status.corrupt;}
    }
    std::sort(segments.begin(),segments.begin()+segment_count,[](const Segment& a,const Segment& b){return a.generation<b.generation;});
    if(segment_count) {
        active=segments[segment_count-1];next_sequence=active.first+active.records;
        StoredRecord last;
        bool tail_bad=active.partial || (active.records && !read_record(active,active.records-1,last));
        if(status.fault)return false;
        if(tail_bad) {
            status.recovered=true;++status.corrupt;
            // Preserve the interrupted segment and advance past its incomplete sequence.
            if(active.partial)++next_sequence;
            if(!create_segment()){fail();return false;}
        }

    }
    for(unsigned slot=0;slot<2;++slot) {
        StoreWire wire;StoredRecord saved;
        if(read_wire(ui_path(slot),0,wire) && store_decode(wire,saved) && saved.kind==StoreKind::Ui && saved.sequence>ui_generation) {ui=saved;ui_generation=saved.sequence;ui_loaded=true;}
        else {SharedSpiGuard g;if(SD.exists(ui_path(slot)))status.recovered=true;}
    }
#ifdef CARDPUTER_ADV_M5
    if(ui_loaded && ui.conversation){StoredRecord latest;if(load_context(ui.conversation,latest) && latest.sequence>ui_generation){ui=latest;ui_generation=latest.sequence;status.recovered=true;}if(status.fault)return false;}
#endif
#ifdef CARDPUTER_ADV_M6
    if(!load_url())return false;
#endif
    unsigned checked=0;
    for(int segment=int(segment_count)-1;segment>=0 && checked<32;--segment)for(int index=int(segments[segment].records)-1;index>=0 && checked<32;--index,++checked) {
        StoredRecord previous;if(read_record(segments[segment],index,previous) && previous.kind==StoreKind::Event && previous.event.kind==MeshEventKind::Text)recent_rx[recent_rx_at++%recent_rx.size()]=store_fingerprint(previous);
    }
    boot=(std::uint64_t(esp_random())<<32)|esp_random();
    StoredRecord session;session.kind=StoreKind::Session;session.boot=boot;session.monotonic=millis();
    if(!append(session)){fail();return false;}
    for(unsigned i=0;i<segment_count;++i)status.records+=segments[i].records;
    // append already counted the session.
    --status.records;status.ready=true;scanning=true;
    if(xTaskCreate(worker,"sd-history",
#ifdef CARDPUTER_ADV_M5
        8192,
#else
        6144,
#endif
        nullptr,1,nullptr)!=pdPASS){fail();return false;}
    return true;
}
#ifdef CARDPUTER_ADV_M8
bool app_storage_flush(std::uint32_t timeout_ms){
    auto start=millis();
    portENTER_CRITICAL(&mux);force_ui=true;portEXIT_CRITICAL(&mux);
    for(;;){portENTER_CRITICAL(&mux);bool fault=status.fault;bool idle=!ui_dirty && !ui_writing && !status.switching;portEXIT_CRITICAL(&mux);
        if(fault)return false;if(idle)return true;if(std::uint32_t(millis()-start)>=timeout_ms)return false;vTaskDelay(pdMS_TO_TICKS(10));}
}
#endif
void app_storage_tick() {} // Worker owns log I/O; loop never scans history.
bool app_storage_event(const MeshEvent& e,std::uint64_t conversation,bool synchronous) {
    StoredRecord r;r.event=e;r.conversation=conversation;r.boot=boot;r.monotonic=millis();
    r.utc=getValidTime(RTCQualityGPS);r.utc_valid=r.utc!=0;
    StoreWire check;if(!store_encode(r,check)){portENTER_CRITICAL(&mux);++status.dropped;portEXIT_CRITICAL(&mux);return false;}
    if(synchronous && (!sync_mutex || xSemaphoreTake(sync_mutex,pdMS_TO_TICKS(1000))!=pdTRUE))return false;
    if(synchronous)while(xSemaphoreTake(sync_done,0)==pdTRUE){}
    portENTER_CRITICAL(&mux);bool ok=status.ready && !status.fault && queue.push(Job{r,synchronous});if(!ok)++status.dropped;portEXIT_CRITICAL(&mux);
    if(synchronous) {
        if(ok) {
            if(xSemaphoreTake(sync_done,pdMS_TO_TICKS(1000))==pdTRUE) {portENTER_CRITICAL(&mux);ok=sync_result;portEXIT_CRITICAL(&mux);}
            else {ok=false;fail();}
        }
        xSemaphoreGive(sync_mutex);
    }
    return ok;
}
void app_storage_ui(const char* draft,std::uint16_t cursor,std::uint8_t view,std::uint64_t conversation,bool force,const UiNavigation* navigation) {
    if(!draft || !mesh_valid_utf8(draft,kMeshTextBytes+1,true) || cursor>std::strlen(draft) || view>3)return;
    portENTER_CRITICAL(&mux);
    ui.kind=StoreKind::Ui;StoredRecord previous=ui;
    if(navigation){
#ifdef CARDPUTER_ADV_M5
        auto read=ui.conversation==conversation?ui.navigation.read_sequence:0;
#endif
        ui.navigation=*navigation;
#ifdef CARDPUTER_ADV_M5
        if(navigation->conversations)ui.navigation.read_sequence=std::max(read,ui.navigation.read_sequence);
#endif
    }
#ifndef CARDPUTER_ADV_M4
    else ui.navigation={};
#endif
    std::strcpy(ui.event.text,draft);ui.cursor=cursor;ui.view=view;ui.conversation=conversation;
    StoreWire old_wire,new_wire;
    bool changed=store_encode(previous,old_wire) && store_encode(ui,new_wire) && old_wire!=new_wire;
    if(changed){if(!ui_dirty)ui_dirty_since=millis();ui_dirty=true;ui_at=millis();}
    if(ui_dirty)force_ui|=force;portEXIT_CRITICAL(&mux);
}
bool app_storage_load_ui(StoredRecord& record) {portENTER_CRITICAL(&mux);record=ui;bool loaded=ui_loaded;portEXIT_CRITICAL(&mux);return loaded;}
bool app_storage_offset(std::int16_t minutes) {
    if(minutes<-720 || minutes>840)return false;
    portENTER_CRITICAL(&mux);if(ui.utc_offset==minutes){portEXIT_CRITICAL(&mux);return true;}ui.utc_offset=minutes;if(!ui_dirty)ui_dirty_since=millis();ui_dirty=true;ui_at=millis();force_ui=true;portEXIT_CRITICAL(&mux);return true;
}
#ifdef CARDPUTER_ADV_M7_NODES
NodeMetricsStamp app_storage_node_metrics(std::uint32_t node){NodeMetricsStamp out;portENTER_CRITICAL(&mux);for(auto& s:metrics_stamps)if(s.node==node){out=s;break;}portEXIT_CRITICAL(&mux);return out;}
#endif
StorageStatus app_storage_status() {portENTER_CRITICAL(&mux);auto copy=status;portEXIT_CRITICAL(&mux);return copy;}
void app_storage_page(std::uint64_t before,std::uint64_t conversation) {portENTER_CRITICAL(&mux);page_conversation=conversation;page_before=before;page_after=false;page_requested=true;status.busy=true;portEXIT_CRITICAL(&mux);}
void app_storage_page_newer(std::uint64_t after,std::uint64_t conversation){portENTER_CRITICAL(&mux);page_conversation=conversation;page_before=after;page_after=true;page_requested=true;status.busy=true;portEXIT_CRITICAL(&mux);}
bool app_storage_page_latest(){portENTER_CRITICAL(&mux);bool latest=page_latest;portEXIT_CRITICAL(&mux);return latest;}
#ifdef CARDPUTER_ADV_M5
bool app_storage_bind_channels(const std::uint64_t* identities,std::size_t count) {
    if(!identities || count>8)return false;
    portENTER_CRITICAL(&mux);bool ok=status.ready && !status.fault;
    if(ok){requested_channels.fill(0);std::copy(identities,identities+count,requested_channels.begin());channels_requested=true;}
    portEXIT_CRITICAL(&mux);return ok;
}
ConversationStatus app_storage_conversation(std::uint8_t channel) {portENTER_CRITICAL(&mux);auto value=channel<8?conversations[channel]:ConversationStatus{};value.scanning|=channels_requested;portEXIT_CRITICAL(&mux);return value;}
bool app_storage_select(std::uint64_t conversation,std::uint8_t channel) {
    if(!conversation || channel>7)return false;
    portENTER_CRITICAL(&mux);bool ok=status.ready && !status.fault && !status.switching;
    if(ok){switch_conversation=conversation;switch_channel=channel;switch_peer=0;status.switching=true;}
    portEXIT_CRITICAL(&mux);return ok;
}
#ifdef CARDPUTER_ADV_M7
bool app_storage_select_peer(std::uint64_t conversation,std::uint8_t channel,std::uint32_t peer) {
    if(!conversation || channel>7 || peer<2 || peer==0xffffffff)return false;
    portENTER_CRITICAL(&mux);bool ok=status.ready && !status.fault && !status.switching;
    if(ok){switch_conversation=conversation;switch_channel=channel;switch_peer=peer;status.switching=true;}
    portEXIT_CRITICAL(&mux);return ok;
}
#endif
void app_storage_mark_read(std::uint64_t conversation,std::uint64_t sequence) {
    portENTER_CRITICAL(&mux);
    for(auto& c:conversations)if(c.identity==conversation && sequence>=c.latest){c.read=std::max(c.read,sequence);c.unread=0;}
    if(ui.conversation==conversation && !status.switching && sequence>ui.navigation.read_sequence){ui.navigation.read_sequence=sequence;if(!ui_dirty)ui_dirty_since=millis();ui_dirty=true;force_ui=true;ui_at=millis();}
    portEXIT_CRITICAL(&mux);
}
#endif
#ifdef CARDPUTER_ADV_M6
bool app_storage_url(const char* text,std::uint16_t cursor,bool export_file){if(!text || !mesh_valid_utf8(text,2048,true) || cursor>std::strlen(text) || (cursor<std::strlen(text) && (std::uint8_t(text[cursor])&0xc0)==0x80))return false;portENTER_CRITICAL(&mux);bool ok=status.ready && !status.fault && !url_export;if(ok && (export_file || std::strcmp(url_draft,text) || url_cursor!=cursor)){if(!url_dirty)url_dirty_since=millis();std::strcpy(url_draft,text);url_cursor=cursor;url_dirty=true;url_export|=export_file;url_at=millis();}portEXIT_CRITICAL(&mux);return ok;}
bool app_storage_url_load(char* text,std::size_t capacity,std::uint16_t& cursor){if(!text || capacity<2048)return false;portENTER_CRITICAL(&mux);std::strcpy(text,url_draft);cursor=url_cursor;portEXIT_CRITICAL(&mux);return true;}
#endif
std::size_t app_storage_page_read(StoredRecord* out,std::size_t capacity) {portENTER_CRITICAL(&mux);auto count=status.busy?0:std::min(capacity,page_count);for(std::size_t i=0;i<count;++i)out[i]=page[i];portEXIT_CRITICAL(&mux);return count;}
}
#endif

#include "cardputer_adv/store_codec.hpp"
#include <cstring>
#include <cmath>
namespace cardputer_adv {
RxFingerprint store_fingerprint(const StoredRecord& r) {return {r.conversation,r.boot,r.event.node,r.event.packet_id,r.monotonic,r.utc,r.utc_valid};}
bool store_same_rx(const RxFingerprint& a,const RxFingerprint& b) {
    if(!a.packet || !a.node || !a.conversation || a.packet!=b.packet || a.node!=b.node || a.conversation!=b.conversation)return false;
    if(a.boot==b.boot)return std::uint32_t(b.monotonic-a.monotonic)<=3600000;
    return a.utc_valid && b.utc_valid && b.utc>=a.utc && b.utc-a.utc<=3600;
}
std::uint32_t store_crc(const std::uint8_t* data,std::size_t size,std::uint32_t initial) {
    auto crc=~initial;
    for(std::size_t i=0;i<size;++i) {crc^=data[i];for(int bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&-(crc&1u));}
    return ~crc;
}
namespace {
void put(StoreWire& w,std::size_t at,std::uint64_t value,unsigned bytes) {for(unsigned i=0;i<bytes;++i)w[at+i]=value>>(i*8);}
std::uint64_t get(const StoreWire& w,std::size_t at,unsigned bytes) {std::uint64_t value=0;for(unsigned i=0;i<bytes;++i)value|=std::uint64_t(w[at+i])<<(i*8);return value;}
void putFloat(StoreWire& w,std::size_t at,float value) {std::uint32_t bits;static_assert(sizeof(value)==4);std::memcpy(&bits,&value,4);put(w,at,bits,4);}
float getFloat(const StoreWire& w,std::size_t at) {auto bits=std::uint32_t(get(w,at,4));float value;std::memcpy(&value,&bits,4);return value;}
}
bool store_encode(const StoredRecord& r,StoreWire& w) {
    if(r.navigation.theme>3 || r.navigation.settings_selection>13)return false;
    if(!mesh_valid_utf8(r.navigation.node_query,sizeof(r.navigation.node_query),true))return false;
    if(r.navigation.chat_peer==1 || r.navigation.chat_peer==0xffffffff)return false;
    if(!mesh_valid_utf8(r.event.text,sizeof(r.event.text),true,r.kind==StoreKind::Event && r.event.kind==MeshEventKind::Text) || unsigned(r.kind)<1 || unsigned(r.kind)>3 ||
       unsigned(r.event.kind)>unsigned(MeshEventKind::Error) || r.cursor>std::strlen(r.event.text) || r.view>3 || r.utc_offset<-720 || r.utc_offset>840 || r.event.channel>7 || unsigned(r.delivery)>unsigned(MeshEventKind::Error) || !std::isfinite(r.event.rssi) || !std::isfinite(r.event.snr) || !std::isfinite(r.event.voltage)) return false;
    w.fill(0);w[0]='A';w[1]='D';w[2]='V';w[3]='3';put(w,4,1,2);put(w,6,kStoreRecordBytes,2);
    w[8]=unsigned(r.kind);w[9]=unsigned(r.event.kind);w[10]=r.utc_valid;w[11]=r.event.channel;
    put(w,12,r.sequence,8);put(w,20,r.boot,8);put(w,28,r.conversation,8);put(w,36,r.monotonic,4);put(w,40,r.utc,4);
    put(w,44,r.event.packet_id,4);put(w,48,r.event.node,4);put(w,52,r.event.destination,4);put(w,56,r.event.token,4);put(w,60,r.event.time,4);
    put(w,64,std::uint32_t(r.event.latitude),4);put(w,68,std::uint32_t(r.event.longitude),4);put(w,72,std::uint32_t(r.event.altitude),4);put(w,76,std::uint32_t(r.event.result),4);
    putFloat(w,80,r.event.rssi);putFloat(w,84,r.event.snr);putFloat(w,88,r.event.voltage);
    w[92]=r.event.battery;w[93]=r.event.has_position|(r.event.has_voltage<<1)|(r.event.has_battery<<2);
    put(w,94,r.cursor,2);put(w,96,std::uint16_t(r.utc_offset),2);w[98]=r.view;w[99]=unsigned(r.delivery);
    auto size=std::strlen(r.event.text);put(w,100,size,2);std::memcpy(w.data()+102,r.event.text,size);
    w[302]=r.event.hop_start;w[303]=r.event.hop_limit;
    w[304]=r.navigation.shell;
    put(w,305,r.navigation.node_offset,4);put(w,309,r.navigation.history_before,8);put(w,317,r.navigation.detail_row,2);
    w[319]=r.navigation.node_selection;w[320]=r.navigation.channel_selection;w[321]=r.navigation.settings_selection>11?11:r.navigation.settings_selection;w[322]=r.navigation.history_selection;
    w[323]=r.navigation.history_first;put(w,324,r.navigation.detail_sequence,8);
    w[332]=r.navigation.conversations;w[333]=r.navigation.chat_channel;put(w,334,r.navigation.read_sequence,8);
    put(w,342,r.navigation.chat_peer,4);
    w[346]=r.navigation.node_favorites;w[347]=std::strlen(r.navigation.node_query);std::memcpy(w.data()+348,r.navigation.node_query,w[347]);
    w[372]=r.navigation.theme;w[373]=r.navigation.settings_selection;
    put(w,380,store_crc(w.data(),380),4);return true;
}
bool store_decode(const StoreWire& w,StoredRecord& r) {
    if(std::memcmp(w.data(),"ADV3",4) || get(w,4,2)!=1 || get(w,6,2)!=kStoreRecordBytes || get(w,380,4)!=store_crc(w.data(),380)) return false;
    auto size=get(w,100,2);if(size>kMeshTextBytes || w[8]<1 || w[8]>3 || w[9]>unsigned(MeshEventKind::Error) || w[10]>1 || w[11]>7 || w[93]>7 || w[98]>3 || w[99]>unsigned(MeshEventKind::Error)) return false;
    r={};r.kind=StoreKind(w[8]);r.event.kind=MeshEventKind(w[9]);r.utc_valid=w[10];r.event.channel=w[11];
    r.sequence=get(w,12,8);r.boot=get(w,20,8);r.conversation=get(w,28,8);r.monotonic=get(w,36,4);r.utc=get(w,40,4);
    r.event.packet_id=get(w,44,4);r.event.node=get(w,48,4);r.event.destination=get(w,52,4);r.event.token=get(w,56,4);r.event.time=get(w,60,4);
    r.event.latitude=std::int32_t(get(w,64,4));r.event.longitude=std::int32_t(get(w,68,4));r.event.altitude=std::int32_t(get(w,72,4));r.event.result=std::int32_t(get(w,76,4));
    r.event.rssi=getFloat(w,80);r.event.snr=getFloat(w,84);r.event.voltage=getFloat(w,88);r.event.battery=w[92];
    r.event.hop_start=w[302];r.event.hop_limit=w[303];
    r.event.has_position=w[93]&1;r.event.has_voltage=w[93]&2;r.event.has_battery=w[93]&4;
    if(w[304]>1 || (w[304] && (w[319]>3 || w[320]>7 || w[321]>11 || w[322]>3 || w[323]>3)))return false;
    r.navigation.shell=w[304];r.navigation.node_offset=get(w,305,4);r.navigation.history_before=get(w,309,8);r.navigation.detail_row=get(w,317,2);
    r.navigation.node_selection=w[319];r.navigation.channel_selection=w[320];r.navigation.settings_selection=w[321];r.navigation.history_selection=w[322];
    r.navigation.history_first=w[323];r.navigation.detail_sequence=get(w,324,8);
    if(w[332]>1 || (w[332] && w[333]>7))return false;
    r.navigation.conversations=w[332];r.navigation.chat_channel=w[333];r.navigation.read_sequence=get(w,334,8);
    r.navigation.chat_peer=get(w,342,4);
    if(r.navigation.theme>3 || r.navigation.settings_selection>13)return false;
    if(!mesh_valid_utf8(r.navigation.node_query,sizeof(r.navigation.node_query),true))return false;
    if(r.navigation.chat_peer==1 || r.navigation.chat_peer==0xffffffff)return false;
    if(w[346]>1 || w[347]>24)return false;
    r.navigation.node_favorites=w[346];std::memcpy(r.navigation.node_query,w.data()+348,w[347]);
    if(std::strlen(r.navigation.node_query)!=w[347] || !mesh_valid_utf8(r.navigation.node_query,sizeof(r.navigation.node_query),true))return false;
    if(w[372]>3 || w[373]>13)return false;
    r.navigation.theme=w[372];if(w[373]>11)r.navigation.settings_selection=w[373];
    r.cursor=get(w,94,2);r.utc_offset=std::int16_t(get(w,96,2));r.view=w[98];r.delivery=MeshEventKind(w[99]);std::memcpy(r.event.text,w.data()+102,size);
    return std::strlen(r.event.text)==size && mesh_valid_utf8(r.event.text,sizeof(r.event.text),true,r.kind==StoreKind::Event && r.event.kind==MeshEventKind::Text) && r.cursor<=size && r.utc_offset>=-720 && r.utc_offset<=840 && std::isfinite(r.event.rssi) && std::isfinite(r.event.snr) && std::isfinite(r.event.voltage);
}
}

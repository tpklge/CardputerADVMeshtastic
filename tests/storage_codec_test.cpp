#include "cardputer_adv/store_codec.hpp"
#include "cardputer_adv/snapshot_codec.hpp"
#include <cassert>
#include <cstring>
#include <limits>
using namespace cardputer_adv;
int main() {
    const char* reference="123456789";
    assert(store_crc(reinterpret_cast<const uint8_t*>(reference),9)==0xcbf43926);
    auto first=store_crc(reinterpret_cast<const uint8_t*>(reference),4);
    assert(store_crc(reinterpret_cast<const uint8_t*>(reference)+4,5,first)==0xcbf43926);
    StoredRecord r;r.sequence=0x123456789abcdef0ULL;r.boot=0xabcdef1234ULL;r.conversation=0x1122334455667788ULL;
    r.event.kind=MeshEventKind::Text;r.event.node=0xf012abcd;r.event.destination=0xffffffff;r.event.latitude=-157000000;r.event.longitude=-560000000;
    r.event.snr=-12.5f;r.event.rssi=-120.f;r.event.has_position=true;r.utc_valid=true;r.utc=1785000000;
    std::memset(r.event.text,'x',200);r.event.text[200]=0;
    StoreWire wire;assert(store_encode(r,wire));StoredRecord decoded;assert(store_decode(wire,decoded));
    assert(decoded.sequence==r.sequence && decoded.boot==r.boot && decoded.conversation==r.conversation && decoded.event.latitude==r.event.latitude);
    assert(decoded.event.snr==r.event.snr && std::strcmp(decoded.event.text,r.event.text)==0);
    for(std::size_t offset=0;offset<wire.size();++offset) {auto corrupt=wire;corrupt[offset]^=1;assert(!store_decode(corrupt,decoded));}
    for(std::size_t end=0;end<wire.size();++end) {StoreWire torn{};std::memcpy(torn.data(),wire.data(),end);assert(!store_decode(torn,decoded));}
    auto newer=wire;newer[4]=2;auto crc=store_crc(newer.data(),380);for(unsigned i=0;i<4;++i)newer[380+i]=crc>>(8*i);assert(!store_decode(newer,decoded));
    r.kind=StoreKind::Ui;std::strcpy(r.event.text,u8"ação 🛰");r.cursor=std::strlen(r.event.text);r.view=3;r.utc_offset=-240;
    assert(store_encode(r,wire) && store_decode(wire,decoded));assert(decoded.cursor==r.cursor && decoded.utc_offset==-240);
    r.navigation.shell=true;r.navigation.node_offset=8;r.navigation.history_before=99;r.navigation.detail_sequence=40;r.navigation.detail_row=2;r.navigation.history_first=1;r.navigation.history_selection=3;r.navigation.settings_selection=11;
    assert(store_encode(r,wire) && store_decode(wire,decoded));assert(decoded.navigation.shell && decoded.navigation.history_before==99 && decoded.navigation.detail_sequence==40 && decoded.navigation.history_first==1);
    r.navigation.conversations=true;r.navigation.chat_channel=7;r.navigation.read_sequence=0x1122334455667788ULL;assert(store_encode(r,wire) && store_decode(wire,decoded));assert(decoded.navigation.chat_channel==7 && decoded.navigation.read_sequence==r.navigation.read_sequence);
    r.navigation.chat_peer=0x12345678;assert(store_encode(r,wire) && store_decode(wire,decoded));assert(decoded.navigation.chat_peer==0x12345678);
    r.navigation.node_favorites=true;std::strcpy(r.navigation.node_query,"Alpha !1234");assert(store_encode(r,wire) && store_decode(wire,decoded));assert(decoded.navigation.node_favorites && !std::strcmp(decoded.navigation.node_query,"Alpha !1234"));
    r.navigation={};assert(store_encode(r,wire) && store_decode(wire,decoded));assert(!decoded.navigation.shell); // legacy M3 record
    r.cursor=201;assert(!store_encode(r,wire));r.cursor=0;r.event.voltage=std::numeric_limits<float>::quiet_NaN();assert(!store_encode(r,wire));
    r={};r.kind=StoreKind::Event;r.event.kind=MeshEventKind::Text;std::strcpy(r.event.text,"Primeira\nSegunda\tlinha");assert(store_encode(r,wire) && store_decode(wire,decoded));assert(!std::strcmp(r.event.text,decoded.event.text));r.kind=StoreKind::Ui;assert(!store_encode(r,wire));
    RxFingerprint old{11,22,33,44,1000,1800000000,true};auto incoming=old;incoming.monotonic+=100;
    assert(store_same_rx(old,incoming));incoming.conversation++;assert(!store_same_rx(old,incoming));incoming=old;incoming.boot++;incoming.utc++;
    assert(store_same_rx(old,incoming));incoming.utc_valid=false;assert(!store_same_rx(old,incoming));incoming=old;incoming.monotonic+=3600001;assert(!store_same_rx(old,incoming));
    SnapshotManifest a,b;a.generation=1;b.generation=2;for(unsigned i=0;i<4;++i)a.size[i]=b.size[i]=100;
    auto wa=snapshot_encode(a),wb=snapshot_encode(b);SnapshotManifest read_a,read_b;
    assert(snapshot_decode(wa,read_a) && snapshot_decode(wb,read_b));assert(snapshot_select(true,read_a,true,read_b)==1);
    for(std::size_t end=0;end<wb.size();++end) {SnapshotWire torn{};std::memcpy(torn.data(),wb.data(),end);bool valid=snapshot_decode(torn,read_b);assert(!valid);assert(snapshot_select(true,read_a,valid,read_b)==0);}
    assert(snapshot_select(false,a,false,b)==-1);assert(snapshot_select(false,a,true,b)==1);
}

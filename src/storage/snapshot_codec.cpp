#include "cardputer_adv/snapshot_codec.hpp"
#include "cardputer_adv/store_codec.hpp"
#include <cstring>
namespace cardputer_adv {
namespace {
void put(SnapshotWire& w,unsigned at,std::uint64_t v,unsigned n) {for(unsigned i=0;i<n;++i)w[at+i]=v>>(8*i);}
std::uint64_t get(const SnapshotWire& w,unsigned at,unsigned n) {std::uint64_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint64_t(w[at+i])<<(8*i);return v;}
}
SnapshotWire snapshot_encode(const SnapshotManifest& m) {
    SnapshotWire w{};std::memcpy(w.data(),"ADS3",4);w[4]=1;w[5]=5;put(w,8,m.generation,8);
    for(unsigned i=0;i<5;++i){put(w,16+8*i,m.size[i],4);put(w,20+8*i,m.crc[i],4);}
    put(w,60,store_crc(w.data(),60),4);return w;
}
bool snapshot_decode(const SnapshotWire& w,SnapshotManifest& m) {
    if(std::memcmp(w.data(),"ADS3",4) || w[4]!=1 || w[5]!=5 || get(w,60,4)!=store_crc(w.data(),60)) return false;
    m={};m.generation=get(w,8,8);if(!m.generation)return false;
    for(unsigned i=0;i<5;++i){m.size[i]=get(w,16+8*i,4);m.crc[i]=get(w,20+8*i,4);if(i<4 && !m.size[i])return false;}
    return true;
}
int snapshot_select(bool va,const SnapshotManifest& a,bool vb,const SnapshotManifest& b) {if(!va && !vb)return -1;if(!vb)return 0;if(!va)return 1;return b.generation>a.generation?1:0;}
}

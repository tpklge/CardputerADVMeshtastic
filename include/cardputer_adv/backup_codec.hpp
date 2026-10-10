#pragma once
#include "snapshot_codec.hpp"
#include "store_codec.hpp"
#include <cstring>
namespace cardputer_adv {
using BackupWire=std::array<std::uint8_t,112>;
constexpr char kBackupCore[]="54e0d8d0ab2ff56b3a9ce967e53f79e49af560fb";
inline BackupWire backup_encode(const SnapshotManifest& m,std::uint32_t node){
    BackupWire w{};std::memcpy(w.data(),"ADB8",4);w[4]=1;w[5]=24;
    std::memcpy(w.data()+8,kBackupCore,40);
    for(unsigned i=0;i<4;++i)w[48+i]=node>>(8*i);
    auto snapshot=snapshot_encode(m);std::memcpy(w.data()+52,snapshot.data(),56);
    auto crc=store_crc(w.data(),108);for(unsigned i=0;i<4;++i)w[108+i]=crc>>(8*i);
    return w;
}
inline bool backup_decode(const BackupWire& w,std::uint32_t node,SnapshotManifest& m){
    if(std::memcmp(w.data(),"ADB8",4) || w[4]!=1 || w[5]!=24 || std::memcmp(w.data()+8,kBackupCore,40))return false;
    std::uint32_t saved=0,crc=0;for(unsigned i=0;i<4;++i){saved|=std::uint32_t(w[48+i])<<(8*i);crc|=std::uint32_t(w[108+i])<<(8*i);}
    if(!node || saved!=node || crc!=store_crc(w.data(),108))return false;
    SnapshotWire snapshot{};std::memcpy(snapshot.data(),w.data()+52,56);auto sc=store_crc(snapshot.data(),60);for(unsigned i=0;i<4;++i)snapshot[60+i]=sc>>(8*i);
    if(!snapshot_decode(snapshot,m))return false;
    for(auto size:m.size)if(!size || size>1024*1024)return false;
    return true;
}
}

#ifdef CARDPUTER_ADV_M2
#include "cardputer_adv/core_storage.hpp"
#include "cardputer_adv/shared_spi.hpp"
#include <FSImpl.h>
#include <SD.h>
#include <atomic>
#include "Throttle.h"
#include <cstring>
#include <cstdio>
#include <memory>
#include <algorithm>
#include "cardputer_adv/snapshot_codec.hpp"
#include "cardputer_adv/store_codec.hpp"
#include <pb_decode.h>
#ifdef CARDPUTER_ADV_M8
#include "cardputer_adv/backup_codec.hpp"
#include <esp_mac.h>
#endif
#include "mesh/generated/meshtastic/deviceonly.pb.h"
#include "mesh/generated/meshtastic/localonly.pb.h"
#include "cardputer_adv/file_replace.hpp"

namespace cardputer_adv {
namespace {
constexpr const char* root = "/CardputerADVMeshtastic/core";
std::atomic<bool> ready{false}, fault{false};
bool existing=false,storage_recovered=false,mounted=false,recovery_compatible=false;
std::uint32_t health_at=0;
String resolve(const char* path) {
    if (!path || path[0] != '/') return String();
    String result(path);
    if (result.indexOf("..") >= 0) return String();
    if (result.startsWith(root) && (result.length() == std::strlen(root) || result[std::strlen(root)] == '/')) return result;
    return String(root) + result;
}
// Keep interrupted or superseded data under unique names, never overwrite it.
bool archive(const String& path) {
    for (unsigned i=0;i<1000;++i) {
        auto target=path+".saved-"+String(i);
        if(!SD.exists(target)) return SD.rename(path,target);
    }
    return false;
}
template<class T> bool validProto(const String& path,const pb_msgdesc_t* fields,size_t maximum) {
    SharedSpiGuard guard;auto f=SD.open(path,FILE_READ);
    if(!f || !f.size() || f.size()>maximum) return false;
    auto size=f.size();
    std::unique_ptr<uint8_t[]> bytes(new(std::nothrow) uint8_t[size]);
    std::unique_ptr<T> value(new(std::nothrow) T{});
    if(!bytes || !value || f.read(bytes.get(),size)!=size) return false;
    f.close();
    auto stream=pb_istream_from_buffer(bytes.get(),size);
    return pb_decode(&stream,fields,value.get()) && value->version==24;
}
template<class T> bool recover(const char* name,const pb_msgdesc_t* fields,size_t maximum) {
    auto base=String(root)+"/prefs/"+name+".proto";
    auto tmp=base+".tmp",backup=base+".bak";
    bool current=SD.exists(base);
    if(current && !validProto<T>(base,fields,maximum)) return false;
    if(!current) {
        // A read-back-verified pending write is preferred; otherwise restore the prior generation.
        auto source=validProto<T>(tmp,fields,maximum)?tmp:backup;
        if(SD.exists(tmp) || SD.exists(backup)) {
            if(!validProto<T>(source,fields,maximum) || !SD.rename(source,base)) return false;
        }
    }
    if(SD.exists(tmp)) {storage_recovered=true;if(!archive(tmp))return false;}
    if(SD.exists(backup) && !archive(backup)) return false;
    return true;
}
struct SdOperations {
    bool exists(const String& p) { return SD.exists(p); }
    bool rename(const String& a,const String& b) { return SD.rename(a,b); }
    bool remove(const String& p) { return SD.remove(p); }
};
#ifdef CARDPUTER_ADV_M3
constexpr const char* snapshot_names[]={"device.proto","config.proto","module.proto","channels.proto","nodes.proto"};
SnapshotManifest committed;
int committed_slot=-1;
SemaphoreHandle_t save_mutex=nullptr;
unsigned save_depth=0;
String snapshot_path(unsigned slot) {return String("/CardputerADVMeshtastic/state/")+(slot?"b":"a");}
class SpiFile:public File {
public:
    SpiFile& operator=(const File& other) {SharedSpiGuard g;File::operator=(other);return *this;}
    ~SpiFile(){SharedSpiGuard g;close();}
};
bool file_digest(const String& path,std::uint32_t& size,std::uint32_t& crc) {
    SpiFile f;{SharedSpiGuard g;f=SD.open(path,FILE_READ);if(!f || f.isDirectory() || f.size()>1024*1024)return false;size=f.size();}
    crc=0;std::uint8_t buffer[256];std::uint32_t done=0;
    while(done<size) {size_t n;{SharedSpiGuard g;n=f.read(buffer,std::min<std::uint32_t>(sizeof(buffer),size-done));}if(!n)return false;crc=store_crc(buffer,n,crc);done+=n;}
    {SharedSpiGuard g;f.close();}return true;
}
bool copy_checked(const String& source,const String& target,std::uint32_t expected_size,std::uint32_t expected_crc) {
    SpiFile in,out;{SharedSpiGuard g;in=SD.open(source,FILE_READ);out=SD.open(target,FILE_WRITE);}
    if(!in || !out)return false;
    std::uint8_t buffer[256];std::uint32_t done=0;
    while(done<expected_size) {size_t n;{SharedSpiGuard g;n=in.read(buffer,std::min<std::uint32_t>(sizeof(buffer),expected_size-done));if(n && out.write(buffer,n)!=n)return false;}if(!n)return false;done+=n;}
    {SharedSpiGuard g;out.flush();out.close();in.close();}
    std::uint32_t size,crc;return file_digest(target,size,crc) && size==expected_size && crc==expected_crc;
}
bool snapshot_valid(unsigned slot,SnapshotManifest& manifest) {
    SnapshotWire wire;{SharedSpiGuard g;auto f=SD.open(snapshot_path(slot)+"/manifest.bin",FILE_READ);if(!f || f.size()!=wire.size() || f.read(wire.data(),wire.size())!=wire.size())return false;}
    if(!snapshot_decode(wire,manifest))return false;
    for(unsigned i=0;i<5;++i)if(manifest.size[i]) {std::uint32_t size,crc;if(!file_digest(snapshot_path(slot)+"/"+snapshot_names[i],size,crc) || size!=manifest.size[i] || crc!=manifest.crc[i])return false;}
    return true;
}
bool snapshot_restore() {
    for(unsigned slot=0;slot<2;++slot) {SharedSpiGuard g;auto f=SD.open(snapshot_path(slot)+"/manifest.bin",FILE_READ);std::uint8_t header[5];if(f && f.read(header,5)==5 && !std::memcmp(header,"ADS3",4) && header[4]>1){recovery_compatible=false;return false;}}
    SnapshotManifest a,b;bool va=snapshot_valid(0,a),vb=snapshot_valid(1,b);
    committed_slot=snapshot_select(va,a,vb,b);
    if(committed_slot<0) {SharedSpiGuard g;return !SD.exists("/CardputerADVMeshtastic/state/m3.started");}
    committed=committed_slot?b:a;
    {SharedSpiGuard g;for(unsigned slot=0;slot<2;++slot)if((slot==0?!va:!vb) && SD.exists(snapshot_path(slot)+"/device.proto"))storage_recovered=true;}
    {SharedSpiGuard g;if(!SD.exists(String(root)+"/prefs") && !SD.mkdir(String(root)+"/prefs"))return false;}
    for(unsigned i=0;i<5;++i) {
        auto target=String(root)+"/prefs/"+snapshot_names[i];
        std::uint32_t size,crc;
        if(!committed.size[i])continue;
        if(file_digest(target,size,crc) && size==committed.size[i] && crc==committed.crc[i])continue;
        storage_recovered=true;
        auto tmp=target+".restore";
        {SharedSpiGuard g;if(SD.exists(tmp) && !archive(tmp))return false;}
        if(!copy_checked(snapshot_path(committed_slot)+"/"+snapshot_names[i],tmp,committed.size[i],committed.crc[i]))return false;
        {SharedSpiGuard g;if(SD.exists(target) && !archive(target))return false;if(!SD.rename(tmp,target))return false;}
    }
    return true;
}
#endif

class LockedFile : public fs::FileImpl {
    File file;
    String virtual_path;
public:
    explicit LockedFile(File f) : file(f), virtual_path(f.path()) {
        if (virtual_path.startsWith(root)) virtual_path.remove(0, std::strlen(root));
        if (virtual_path.isEmpty()) virtual_path = "/";
    }
    ~LockedFile() override { close(); }
    size_t write(const uint8_t* b, size_t n) override { SharedSpiGuard g; if (fault) return 0; auto r=file.write(b,n); if(r!=n) fault=true; return r; }
    size_t read(uint8_t* b, size_t n) override { SharedSpiGuard g; auto r=file.read(b,n); if(r>n || (!r && file.position()<file.size())) {fault=true;return 0;} return r; }
    void flush() override { SharedSpiGuard g; file.flush(); }
    bool seek(uint32_t p, SeekMode m) override { SharedSpiGuard g; return file.seek(p,m); }
    size_t position() const override { SharedSpiGuard g; return file.position(); }
    size_t size() const override { SharedSpiGuard g; return file.size(); }
    bool setBufferSize(size_t n) override { SharedSpiGuard g; return file.setBufferSize(n); }
    void close() override { SharedSpiGuard g; file.close(); }
    time_t getLastWrite() override { SharedSpiGuard g; return file.getLastWrite(); }
    const char* path() const override { return virtual_path.c_str(); }
    const char* name() const override { auto p=std::strrchr(virtual_path.c_str(),'/'); return p?p+1:virtual_path.c_str(); }
    boolean isDirectory() override { SharedSpiGuard g; return file.isDirectory(); }
    fs::FileImplPtr openNextFile(const char* mode) override { SharedSpiGuard g; auto f=file.openNextFile(mode); return f?std::make_shared<LockedFile>(f):nullptr; }
    boolean seekDir(long p) override { SharedSpiGuard g; return file.seekDir(p); }
    String getNextFileName() override { bool dir; return getNextFileName(&dir); }
    String getNextFileName(bool* dir) override { SharedSpiGuard g; auto s=file.getNextFileName(dir); if(s.startsWith(root)) s.remove(0,std::strlen(root)); return s; }
    void rewindDirectory() override { SharedSpiGuard g; file.rewindDirectory(); }
    operator bool() override { SharedSpiGuard g; return bool(file); }
};
class CoreFSImpl : public fs::FSImpl {
public:
    fs::FileImplPtr open(const char* path,const char* mode,const bool create) override {
        SharedSpiGuard g; auto p=resolve(path); if(!ready || p.isEmpty() || (fault && mode[0]!='r')) return nullptr;
        auto f=SD.open(p,mode,create); if(!f && mode[0]!='r') fault=true;
        return f?std::make_shared<LockedFile>(f):nullptr;
    }
    bool exists(const char* p) override { SharedSpiGuard g; auto q=resolve(p); return ready&&!q.isEmpty()&&SD.exists(q); }
    bool rename(const char* a,const char* b) override { SharedSpiGuard g; auto x=resolve(a),y=resolve(b); SdOperations ops; bool ok=ready&&!fault&&!x.isEmpty()&&!y.isEmpty()&&replace_file(ops,x,y,String(y+".bak")); if(!ok) fault=true; return ok; }
    bool remove(const char* p) override { SharedSpiGuard g; auto q=resolve(p); return ready&&!fault&&!q.isEmpty()&&SD.remove(q); }
    bool mkdir(const char* p) override { SharedSpiGuard g; auto q=resolve(p); bool ok=ready&&!fault&&!q.isEmpty()&&(SD.exists(q)||SD.mkdir(q)); if(!ok) fault=true; return ok; }
    bool rmdir(const char* p) override { SharedSpiGuard g; auto q=resolve(p); return ready&&!fault&&!q.isEmpty()&&q!=root&&SD.rmdir(q); }
};
}
CoreFilesystem core_filesystem;
CoreFilesystem::CoreFilesystem():FS(std::make_shared<CoreFSImpl>()) {}
bool CoreFilesystem::begin() { return ready && !fault; }
std::uint64_t CoreFilesystem::totalBytes() { SharedSpiGuard g; return SD.totalBytes(); }
std::uint64_t CoreFilesystem::usedBytes() { SharedSpiGuard g; return SD.usedBytes(); }
bool core_storage_fault() { return fault.load(); }
bool core_storage_existing() { return existing; }
bool core_storage_recovered() {return storage_recovered;}
void core_storage_health_tick() {
    if (!ready || fault || Throttle::isWithinTimespanMs(health_at,2000)) return;
    health_at=millis(); SharedSpiGuard g;
    auto f=SD.open(String(root)+"/prefs/config.proto",FILE_READ);
    if(!f || !f.size() || f.read()<0) fault=true;
    f.close();
}
void core_storage_set_fault() { fault=true; }
void core_storage_save_lock() {
#ifdef CARDPUTER_ADV_M3
    if(save_mutex){xSemaphoreTakeRecursive(save_mutex,portMAX_DELAY);++save_depth;}
#endif
}
void core_storage_save_unlock() {
#ifdef CARDPUTER_ADV_M3
    if(save_mutex){--save_depth;xSemaphoreGiveRecursive(save_mutex);}
#endif
}
bool core_storage_commit() {
#ifdef CARDPUTER_ADV_M3
    if(!ready || fault)return false;
    if(save_depth>1)return true;
    SnapshotManifest next;
    for(unsigned i=0;i<5;++i) {
        auto path=String(root)+"/prefs/"+snapshot_names[i];
        {SharedSpiGuard g;if(!SD.exists(path)) {if(i<4)return committed_slot<0;continue;}}
        if(!file_digest(path,next.size[i],next.crc[i])) {fault=true;return false;}
    }
    if(committed_slot>=0 && next.size==committed.size && next.crc==committed.crc)return true;
    auto slot=committed_slot==0?1:0;
    next.generation=committed.generation+1;
    auto base=snapshot_path(slot);
    {SharedSpiGuard g;
        for(auto path:{String("/CardputerADVMeshtastic/state"),base})if(!SD.exists(path) && !SD.mkdir(path)){fault=true;return false;}
        // Once a complete snapshot exists, boot cannot fall back to loose files.
        if(committed_slot>=0 && !SD.exists("/CardputerADVMeshtastic/state/m3.started")) {fault=true;return false;}
        if(SD.exists(base+"/manifest.bin") && !SD.remove(base+"/manifest.bin")){fault=true;return false;}
    }
    for(unsigned i=0;i<5;++i)if(next.size[i]) {
        std::uint32_t size,crc;
        auto target=base+"/"+snapshot_names[i];
        if(file_digest(target,size,crc) && size==next.size[i] && crc==next.crc[i])continue;
        if(!copy_checked(String(root)+"/prefs/"+snapshot_names[i],target,next.size[i],next.crc[i])) {fault=true;return false;}
    }
    auto wire=snapshot_encode(next);
    {SharedSpiGuard g;auto f=SD.open(base+"/manifest.bin",FILE_WRITE);if(!f || f.write(wire.data(),wire.size())!=wire.size()){fault=true;return false;}f.flush();f.close();}
    SnapshotManifest verified;if(!snapshot_valid(slot,verified)){fault=true;return false;}
    {SharedSpiGuard g;if(!SD.exists("/CardputerADVMeshtastic/state/m3.started")){auto f=SD.open("/CardputerADVMeshtastic/state/m3.started",FILE_WRITE);if(!f || f.write(uint8_t(1))!=1){fault=true;return false;}f.flush();f.close();}}
    committed=next;committed_slot=slot;
#endif
    return true;
}


#ifdef CARDPUTER_ADV_M8
namespace {
String backup_path(unsigned slot){char name[80];std::snprintf(name,sizeof(name),"/CardputerADVMeshtastic/backups/config-%03u",slot);return String(name);}
bool backup_nodes_valid(const String& path){
    SpiFile file;std::size_t size;
    {SharedSpiGuard g;file=SD.open(path,FILE_READ);if(!file || !(size=file.size()) || size>200*(meshtastic_NodeInfoLite_size+8)+64)return false;}
    std::unique_ptr<std::uint8_t[]> bytes(new(std::nothrow) std::uint8_t[size]);if(!bytes)return false;
    for(std::size_t at=0;at<size;){SharedSpiGuard g;auto n=file.read(bytes.get()+at,std::min<std::size_t>(256,size-at));if(!n)return false;at+=n;}
    auto stream=pb_istream_from_buffer(bytes.get(),size);bool has_version=false;unsigned nodes=0;
    while(stream.bytes_left){pb_wire_type_t type;std::uint32_t tag;bool eof;if(!pb_decode_tag(&stream,&type,&tag,&eof))return false;
        if(tag==1){std::uint32_t version;if(type!=PB_WT_VARINT || has_version || !pb_decode_varint32(&stream,&version) || version!=24)return false;has_version=true;}
        else if(tag==2){pb_istream_t sub;meshtastic_NodeInfoLite value{};if(type!=PB_WT_STRING || ++nodes>200 || !pb_make_string_substream(&stream,&sub))return false;bool ok=pb_decode(&sub,&meshtastic_NodeInfoLite_msg,&value);if(!pb_close_string_substream(&stream,&sub) || !ok)return false;}
        else if(!pb_skip_field(&stream,type))return false;
    }
    return has_version;
}
bool backup_valid(unsigned slot,std::uint32_t node,SnapshotManifest& manifest){
    if(slot>=kBackupSlots)return false;
    BackupWire wire;{SharedSpiGuard g;auto f=SD.open(backup_path(slot)+"/manifest.bin",FILE_READ);if(!f || f.size()!=wire.size() || f.read(wire.data(),wire.size())!=wire.size())return false;}
    if(!backup_decode(wire,node,manifest))return false;
    for(unsigned i=0;i<5;++i){std::uint32_t size,crc;if(!file_digest(backup_path(slot)+"/"+snapshot_names[i],size,crc) || size!=manifest.size[i] || crc!=manifest.crc[i])return false;}
    // Validate protobuf version as well as CRC. No key material is printed.
    auto base=backup_path(slot)+"/";
    return validProto<meshtastic_DeviceState>(base+"device.proto",&meshtastic_DeviceState_msg,meshtastic_DeviceState_size) &&
        validProto<meshtastic_LocalConfig>(base+"config.proto",&meshtastic_LocalConfig_msg,meshtastic_LocalConfig_size) &&
        validProto<meshtastic_LocalModuleConfig>(base+"module.proto",&meshtastic_LocalModuleConfig_msg,meshtastic_LocalModuleConfig_size) &&
        validProto<meshtastic_ChannelFile>(base+"channels.proto",&meshtastic_ChannelFile_msg,meshtastic_ChannelFile_size) &&
        backup_nodes_valid(base+"nodes.proto");
}
}
bool core_storage_recovery(){return mounted && recovery_compatible && fault;}
bool core_storage_backup_exists(unsigned slot){SharedSpiGuard g;return mounted && recovery_compatible && slot<kBackupSlots && SD.exists(backup_path(slot)+"/manifest.bin");}
bool core_storage_backup(unsigned& slot,std::uint32_t node){
    CoreSaveGuard save;
    if(!ready || fault || !node || !core_storage_commit() || committed_slot<0)return false;
    for(auto size:committed.size)if(!size)return false;
    {SharedSpiGuard g;
        auto parent="/CardputerADVMeshtastic/backups";if(!SD.exists(parent) && !SD.mkdir(parent))return false;
        for(slot=0;slot<kBackupSlots && SD.exists(backup_path(slot));++slot){} // Never reuse even an interrupted backup.
        if(slot==kBackupSlots || !SD.mkdir(backup_path(slot)))return false;
    }
    for(unsigned i=0;i<5;++i)if(!copy_checked(snapshot_path(committed_slot)+"/"+snapshot_names[i],backup_path(slot)+"/"+snapshot_names[i],committed.size[i],committed.crc[i]))return false;
    auto wire=backup_encode(committed,node);
    {SharedSpiGuard g;auto f=SD.open(backup_path(slot)+"/manifest.bin",FILE_WRITE);if(!f || f.write(wire.data(),wire.size())!=wire.size())return false;f.flush();f.close();}
    SnapshotManifest verified;return backup_valid(slot,node,verified);
}
bool core_storage_restore_backup(unsigned slot,std::uint32_t node){
    CoreSaveGuard save;
    SnapshotManifest next;if(!mounted || !recovery_compatible || committed.generation==UINT64_MAX)return false;
    if(!node){
        // Recover without starting the radio. Check the actual hardware MAC against
        // the stored owner, then use that verified device's node number.
        BackupWire wire;{SharedSpiGuard g;auto f=SD.open(backup_path(slot)+"/manifest.bin",FILE_READ);if(!f || f.size()!=wire.size() || f.read(wire.data(),wire.size())!=wire.size())return false;}
        for(unsigned i=0;i<4;++i)node|=std::uint32_t(wire[48+i])<<(8*i);
        if(!backup_valid(slot,node,next))return false;
        meshtastic_DeviceState device{};std::uint8_t mac[6];
        {SharedSpiGuard g;auto f=SD.open(backup_path(slot)+"/device.proto",FILE_READ);std::uint8_t bytes[meshtastic_DeviceState_size];auto size=f.size();if(!f || size>sizeof(bytes) || f.read(bytes,size)!=size)return false;auto stream=pb_istream_from_buffer(bytes,size);if(!pb_decode(&stream,&meshtastic_DeviceState_msg,&device))return false;}
        if(esp_efuse_mac_get_default(mac)!=ESP_OK || !device.has_owner || std::memcmp(device.owner.macaddr,mac,6) || device.my_node.my_node_num!=node)return false;
    } else if(!backup_valid(slot,node,next))return false;
    auto target=committed_slot==0?1:0;auto base=snapshot_path(target);
    {SharedSpiGuard g;if(!SD.exists("/CardputerADVMeshtastic/state") && !SD.mkdir("/CardputerADVMeshtastic/state"))return false;if(!SD.exists(base) && !SD.mkdir(base))return false;if(SD.exists(base+"/manifest.bin") && !SD.remove(base+"/manifest.bin"))return false;}
    for(unsigned i=0;i<5;++i)if(!copy_checked(backup_path(slot)+"/"+snapshot_names[i],base+"/"+snapshot_names[i],next.size[i],next.crc[i]))return false;
    next.generation=std::max(committed.generation,next.generation)+1;if(!next.generation)return false;auto wire=snapshot_encode(next);
    {SharedSpiGuard g;auto f=SD.open(base+"/manifest.bin",FILE_WRITE);if(!f || f.write(wire.data(),wire.size())!=wire.size())return false;f.flush();f.close();}
    SnapshotManifest verified;
    if(!snapshot_valid(target,verified))return false;
    {SharedSpiGuard g;auto f=SD.open("/CardputerADVMeshtastic/state/m3.started",FILE_WRITE);if(!f || f.write(uint8_t(1))!=1)return false;f.flush();f.close();}
    // Prevent any save in the current session from replacing the restored generation.
    // Caller restarts immediately; next boot restores the verified files normally.
    fault=true;
    return true;
}
bool core_storage_diagnostic(const char* text,char* filename,std::size_t capacity,bool traceroute){
    if(!ready || fault || !text || std::strlen(text)>2048)return false;
    unsigned slot;
    {SharedSpiGuard g;auto parent="/CardputerADVMeshtastic/exports";if(!SD.exists(parent) && !SD.mkdir(parent))return false;
        for(slot=0;slot<1000;++slot){std::snprintf(filename,capacity,"%s/%s-%03u.txt",parent,traceroute?"traceroute":"diagnostic",slot);if(!SD.exists(filename))break;}
        if(slot==1000)return false;
        auto f=SD.open(filename,FILE_WRITE);auto size=std::strlen(text);if(!f || f.write(reinterpret_cast<const std::uint8_t*>(text),size)!=size)return false;f.flush();f.close();
    }
    std::uint32_t size,crc;return file_digest(String(filename),size,crc) && size==std::strlen(text) && crc==store_crc(reinterpret_cast<const std::uint8_t*>(text),size);
}
#endif

bool core_storage_mount() {
    SharedSpiGuard g;
    if (!SD.begin(12,SPI,4000000,"/sd",8,false) || SD.cardType()==CARD_NONE) return false;
    mounted=true;
    for(auto p : {"/CardputerADVMeshtastic",root}) if(!SD.exists(p)&&!SD.mkdir(p)) return false;
#ifdef CARDPUTER_ADV_M3
    {
        auto f=SD.open("/CardputerADVMeshtastic/schema.bin",FILE_READ);std::uint8_t header[5];
        if(f && (f.read(header,5)!=5 || std::memcmp(header,"ADP3",4) || header[4]!=1)) {fault=true;return false;}
    }
    recovery_compatible=true;
    save_mutex=xSemaphoreCreateRecursiveMutex();
    if(!save_mutex || !snapshot_restore()) {fault=true;return false;}
#endif
    if(!recover<meshtastic_DeviceState>("device",&meshtastic_DeviceState_msg,meshtastic_DeviceState_size) ||
       !recover<meshtastic_LocalConfig>("config",&meshtastic_LocalConfig_msg,meshtastic_LocalConfig_size) ||
       !recover<meshtastic_LocalModuleConfig>("module",&meshtastic_LocalModuleConfig_msg,meshtastic_LocalModuleConfig_size) ||
       !recover<meshtastic_ChannelFile>("channels",&meshtastic_ChannelFile_msg,meshtastic_ChannelFile_size)) {fault=true;return false;}
    unsigned found=0;
    for(auto name : {"device", "config", "module", "channels"}) {
        auto base=String(root)+"/prefs/"+name+".proto";
        if(SD.exists(base)) ++found;
        if(SD.exists(base+".tmp")) {fault=true;return false;}
    }
    if((found && found!=4) || SD.exists(String(root)+"/prefs/db.proto")) {fault=true;return false;}
    if(!found && (SD.exists(String(root)+"/prefs/nodes.proto") || SD.exists(String(root)+"/prefs/uiconfig.proto"))) {fault=true;return false;}
    existing=found==4;
    auto f=SD.open(String(root)+"/.write-probe",FILE_WRITE);
    if(!f) return false;
    bool ok=f.write(static_cast<uint8_t>(0x5a))==1; f.flush(); f.close();
    f=SD.open(String(root)+"/.write-probe",FILE_READ); ok=ok&&f&&f.size()==1&&f.read()==0x5a; f.close();
    SD.remove(String(root)+"/.write-probe"); ready=ok;
#ifdef CARDPUTER_ADV_M3
    if(ok && existing && committed_slot<0) {ok=core_storage_commit();if(!ok)fault=true;}
#endif
    return ok;
}
}
#endif

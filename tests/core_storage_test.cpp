#include <cassert>
#include <pb_encode.h>
#include <vector>
#include "core_storage_native.cpp"
using namespace cardputer_adv;
// Backup validation uses a bounded streaming decoder, never the vector callback.
bool meshtastic_NodeDatabase_callback(pb_istream_t*,pb_ostream_t*,const pb_field_iter_t*){assert(false);return false;}
template<class T> std::vector<std::uint8_t> encode(const T& v,const pb_msgdesc_t* fields){std::vector<std::uint8_t> out(4096);auto stream=pb_ostream_from_buffer(out.data(),out.size());assert(pb_encode(&stream,fields,&v));out.resize(stream.bytes_written);return out;}
void write_file(const std::string& name,const std::vector<std::uint8_t>& data){SD.files[name]=std::make_shared<std::vector<std::uint8_t>>(data);}
void reboot(){ready=false;fault=false;committed={};committed_slot=-1;storage_recovered=false;save_depth=0;assert(core_storage_mount());}
int main(){
    meshtastic_DeviceState device{};device.version=24;device.has_owner=true;device.has_my_node=true;device.my_node.my_node_num=0x1234;const std::uint8_t mac[6]={1,2,3,4,5,6};std::memcpy(device.owner.macaddr,mac,6);meshtastic_LocalConfig cfg{};cfg.version=24;cfg.has_lora=true;cfg.lora.hop_limit=3;meshtastic_LocalModuleConfig modules{};modules.version=24;meshtastic_ChannelFile ch{};ch.version=24;
    auto base=std::string("/CardputerADVMeshtastic/core/prefs/");
    write_file(base+"device.proto",encode(device,&meshtastic_DeviceState_msg));write_file(base+"config.proto",encode(cfg,&meshtastic_LocalConfig_msg));write_file(base+"module.proto",encode(modules,&meshtastic_LocalModuleConfig_msg));write_file(base+"channels.proto",encode(ch,&meshtastic_ChannelFile_msg));write_file(base+"nodes.proto",{8,24});
    write_file("/CardputerADVMeshtastic/messages/test.log",{1,2,3});write_file("/CardputerADVMeshtastic/drafts/test.bin",{4,5,6});
    assert(core_storage_mount());unsigned slot=99;assert(core_storage_backup(slot,0x1234) && slot==0 && core_storage_backup_exists(0));auto original=*SD.files.at(base+"config.proto");
    cfg.lora.hop_limit=7;write_file(base+"config.proto",encode(cfg,&meshtastic_LocalConfig_msg));{CoreSaveGuard guard;assert(core_storage_commit());}auto changed=*SD.files.at(base+"config.proto");assert(original!=changed);
    assert(!core_storage_restore_backup(0,0x5678));assert(*SD.files.at(base+"config.proto")==changed);
    auto manifest=std::string("/CardputerADVMeshtastic/backups/config-000/manifest.bin");(*SD.files.at(manifest))[9]^=1;assert(!core_storage_restore_backup(0,0x1234));(*SD.files.at(manifest))[9]^=1;
    fail_write_path="/CardputerADVMeshtastic/backups/config-001/module.proto";fail_after=1;assert(!core_storage_backup(slot,0x1234) && !core_storage_backup_exists(1));fail_write_path.clear();fail_after=-1;assert(core_storage_backup(slot,0x1234) && slot==2); // Interrupted names are not reused.
    auto target=snapshot_path(committed_slot==0?1:0);fail_write_path=target+"/module.proto";fail_after=1;assert(!core_storage_restore_backup(0,0x1234));fail_write_path.clear();fail_after=-1;reboot();assert(*SD.files.at(base+"config.proto")==changed);
    // A torn manifest cannot supersede the last durable generation.
    target=snapshot_path(committed_slot==0?1:0);fail_write_path=target+"/manifest.bin";fail_after=20;assert(!core_storage_restore_backup(0,0x1234));fail_write_path.clear();fail_after=-1;reboot();assert(*SD.files.at(base+"config.proto")==changed);
    assert(core_storage_restore_backup(0,0x1234) && core_storage_fault());assert(!core_storage_commit());reboot();assert(*SD.files.at(base+"config.proto")==original);
    assert(*SD.files.at("/CardputerADVMeshtastic/messages/test.log")==std::vector<std::uint8_t>({1,2,3}));assert(*SD.files.at("/CardputerADVMeshtastic/drafts/test.bin")==std::vector<std::uint8_t>({4,5,6}));
    // Reuse unchanged files in the inactive generation, with a new manifest.
    cfg.lora.hop_limit=4;write_file(base+"config.proto",encode(cfg,&meshtastic_LocalConfig_msg));{CoreSaveGuard guard;assert(core_storage_commit());}
    cfg.lora.hop_limit=5;write_file(base+"config.proto",encode(cfg,&meshtastic_LocalConfig_msg));auto destination=std::string(snapshot_path(committed_slot==0?1:0));auto writes=file_writes[destination+"/module.proto"];{CoreSaveGuard guard;assert(core_storage_commit());}assert(file_writes[destination+"/module.proto"]==writes);
    // A failed checkpoint write cannot publish a partial generation.
    cfg.lora.hop_limit=6;write_file(base+"config.proto",encode(cfg,&meshtastic_LocalConfig_msg));destination=std::string(snapshot_path(committed_slot==0?1:0));fail_write_path=destination+"/config.proto";fail_after=1;
    {CoreSaveGuard guard;assert(!core_storage_commit() && core_storage_fault());}fail_write_path.clear();fail_after=-1;reboot();cfg.lora.hop_limit=5;assert(*SD.files.at(base+"config.proto")==encode(cfg,&meshtastic_LocalConfig_msg));
    char filename[100];assert(core_storage_diagnostic("No secrets\n",filename,sizeof(filename)));auto first=std::string(filename);assert(core_storage_diagnostic("No secrets\n",filename,sizeof(filename)) && first!=filename);
    // Both checkpoints are unreadable: the backup remains reachable without core startup.
    for(unsigned i=0;i<2;++i){auto name=std::string(snapshot_path(i))+"/manifest.bin";if(SD.files.count(name))(*SD.files.at(name))[0]^=1;}
    ready=false;fault=false;committed={};committed_slot=-1;assert(!core_storage_mount());assert(core_storage_recovery() && core_storage_backup_exists(0));
    auto saved_device=*SD.files.at("/CardputerADVMeshtastic/backups/config-000/device.proto");
    device.owner.macaddr[0]=9;auto wrong=encode(device,&meshtastic_DeviceState_msg);write_file("/CardputerADVMeshtastic/backups/config-000/device.proto",wrong);
    auto saved_manifest=*SD.files.at(manifest);BackupWire wrong_wire;std::memcpy(wrong_wire.data(),saved_manifest.data(),wrong_wire.size());SnapshotManifest wrong_snapshot;assert(backup_decode(wrong_wire,0x1234,wrong_snapshot));wrong_snapshot.size[0]=wrong.size();wrong_snapshot.crc[0]=store_crc(wrong.data(),wrong.size());wrong_wire=backup_encode(wrong_snapshot,0x1234);write_file(manifest,std::vector<std::uint8_t>(wrong_wire.begin(),wrong_wire.end()));
    assert(!core_storage_restore_backup(0,0));write_file(manifest,saved_manifest);write_file("/CardputerADVMeshtastic/backups/config-000/device.proto",saved_device);
    assert(core_storage_restore_backup(0,0));reboot();assert(*SD.files.at(base+"config.proto")==original);
    assert(*SD.files.at("/CardputerADVMeshtastic/drafts/test.bin")==std::vector<std::uint8_t>({4,5,6}));
}

#include "cardputer_adv/maintenance.hpp"
#include "cardputer_adv/core_storage.hpp"
#include <Arduino.h>
#include <cassert>
using namespace cardputer_adv;
namespace {
bool recovery=true,fault=true,flush_ok=false;
unsigned flushes=0,restores=0;
std::uint32_t restored_node=0;
StorageStatus storage;
MeshSnapshot snapshot;
}
namespace cardputer_adv {
bool core_storage_recovery(){return recovery;}
bool core_storage_fault(){return fault;}
StorageStatus app_storage_status(){return storage;}
MeshSnapshot mesh_snapshot(){return snapshot;}
TraceSnapshot mesh_trace_snapshot(){return {};}
bool app_storage_flush(std::uint32_t){++flushes;return flush_ok;}
bool core_storage_backup(unsigned&,std::uint32_t){return true;}
bool core_storage_restore_backup(unsigned slot,std::uint32_t node){assert(slot==3);++restores;restored_node=node;return true;}
bool core_storage_diagnostic(const char*,char*,std::size_t,bool){return true;}
}
int main(){
    assert(maintenance_begin());
    assert(!maintenance_submit(MaintenanceAction::Backup));
    assert(!maintenance_submit(MaintenanceAction::Diagnostic));
    assert(maintenance_submit(MaintenanceAction::Restore,3));
    assert(maintenance_busy() && !maintenance_submit(MaintenanceAction::Restore,3));
    storage_step();MaintenanceResult result;assert(maintenance_poll(result) && result.success && result.action==MaintenanceAction::Restore);
    assert(restores==1 && restored_node==0 && flushes==0 && !maintenance_busy());
    recovery=false;fault=false;storage.ready=true;snapshot.ready=true;snapshot.node=0x1234;
    assert(!maintenance_submit(MaintenanceAction::Restore,3));assert(flushes==1);
    flush_ok=true;assert(maintenance_submit(MaintenanceAction::Restore,3));storage_step();assert(maintenance_poll(result) && result.success && restored_node==0x1234 && flushes==2);
}

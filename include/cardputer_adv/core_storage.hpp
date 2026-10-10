#pragma once
#include <FS.h>
#include <cstdint>
namespace cardputer_adv {
class CoreFilesystem : public fs::FS {
public:
    CoreFilesystem();
    bool begin();
    bool format() { return false; }
    std::uint64_t totalBytes();
    std::uint64_t usedBytes();
};
extern CoreFilesystem core_filesystem;
bool core_storage_mount();
bool core_storage_fault();
bool core_storage_existing();
bool core_storage_recovered();
void core_storage_health_tick();
void core_storage_set_fault();
bool core_storage_commit();
#ifdef CARDPUTER_ADV_M8
constexpr unsigned kBackupSlots=64;
bool core_storage_recovery();
bool core_storage_backup(unsigned& slot,std::uint32_t node);
bool core_storage_backup_exists(unsigned slot);
bool core_storage_restore_backup(unsigned slot,std::uint32_t node);
bool core_storage_diagnostic(const char* text,char* filename,std::size_t capacity,bool traceroute=false);
#endif
void core_storage_save_lock();
void core_storage_save_unlock();
struct CoreSaveGuard {CoreSaveGuard(){core_storage_save_lock();}~CoreSaveGuard(){core_storage_save_unlock();}};
}

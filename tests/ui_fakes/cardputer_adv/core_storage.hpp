#pragma once
namespace cardputer_adv {inline bool fake_recovery=false;inline bool core_storage_recovery(){return fake_recovery;}constexpr unsigned kBackupSlots=64;bool core_storage_backup_exists(unsigned slot);}

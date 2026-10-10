#pragma once
#include "app_storage.hpp"
#include <cstdio>
namespace cardputer_adv {
inline int diagnostic_text(char* out,std::size_t capacity,const char* version,const MeshSnapshot& s,const StorageStatus& st){
    // Explicit allowlist: no configuration object, PSK, private/public key, URL,
    // message, owner name or coordinates ever enter this report.
    return std::snprintf(out,capacity,
        "Cardputer ADV Meshtastic %s\nCore 54e0d8d0ab2ff56b3a9ce967e53f79e49af560fb\n"
        "schema=1\nnode=%08lx\nregion=%u\npreset=%u\nhops=%u\nslot=%u\n"
        "radio_ready=%u\ncore_ready=%u\nsd_ready=%u\nsd_fault=%u\nsd_recovered=%u\n"
        "tx=%lu\nrx=%lu\nrx_errors=%lu\nevent_drops=%lu\ncommand_drops=%lu\n"
        "nodes=%lu\nheap_min=%lu\nbattery_mv=%u\ngps_time=%u\ngps_fix=%u\nsatellites=%u\n"
        "heap_free=%lu\nloop_max_ms=%lu\nuptime_ms=%lu\nrecords=%llu\nretired=%llu\nstorage_drops=%lu\nstorage_crc=%lu\n",
        version,static_cast<unsigned long>(s.node),s.region,s.preset,s.hop_limit,s.frequency_slot,
        s.radio_ready,s.ready,st.ready,s.storage_fault||st.fault,st.recovered,
        static_cast<unsigned long>(s.tx),static_cast<unsigned long>(s.rx),static_cast<unsigned long>(s.rx_errors),static_cast<unsigned long>(s.event_drops),static_cast<unsigned long>(s.command_drops),
        static_cast<unsigned long>(s.nodes),static_cast<unsigned long>(s.heap_min),s.battery_mv,s.gps_time,s.gps_fix,s.satellites,
        static_cast<unsigned long>(s.heap_free),static_cast<unsigned long>(s.loop_max_ms),static_cast<unsigned long>(s.uptime_ms),static_cast<unsigned long long>(st.records),static_cast<unsigned long long>(st.retired),static_cast<unsigned long>(st.dropped),static_cast<unsigned long>(st.corrupt));
}
#ifdef CARDPUTER_ADV_M8
enum class MaintenanceAction:std::uint8_t {Diagnostic,Backup,Restore,Trace};
struct MaintenanceResult {MaintenanceAction action=MaintenanceAction::Diagnostic;bool success=false;unsigned slot=0;std::uint32_t duration_ms=0;char filename[100]{};};
bool maintenance_begin();
bool maintenance_submit(MaintenanceAction action,unsigned slot=0);
bool maintenance_busy();
bool maintenance_poll(MaintenanceResult& result);
#endif
}

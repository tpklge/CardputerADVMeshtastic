#pragma once
#include "mesh_backend.hpp"
#include "ui_shell.hpp"
#include <array>
namespace cardputer_adv {
constexpr std::size_t kStoreRecordBytes=384;
using StoreWire=std::array<std::uint8_t,kStoreRecordBytes>;
enum class StoreKind:std::uint8_t { Event=1,Ui=2,Session=3 };
struct StoredRecord {
    StoreKind kind=StoreKind::Event;
    std::uint64_t sequence=0,boot=0,conversation=0;
    std::uint32_t monotonic=0,utc=0;
    bool utc_valid=false;
    MeshEvent event{};
    std::uint16_t cursor=0;
    std::int16_t utc_offset=-240;
    std::uint8_t view=0;
    UiNavigation navigation{};
    MeshEventKind delivery=MeshEventKind::Queued;
};
struct RxFingerprint {std::uint64_t conversation=0,boot=0;std::uint32_t node=0,packet=0,monotonic=0,utc=0;bool utc_valid=false;};
RxFingerprint store_fingerprint(const StoredRecord& record);
bool store_same_rx(const RxFingerprint& previous,const RxFingerprint& incoming);
std::uint32_t store_crc(const std::uint8_t* data,std::size_t size,std::uint32_t initial=0);
bool store_encode(const StoredRecord& record,StoreWire& wire);
bool store_decode(const StoreWire& wire,StoredRecord& record);
}

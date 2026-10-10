#pragma once
#include <array>
#include <cstdint>
namespace cardputer_adv {
struct SnapshotManifest {std::uint64_t generation=0;std::array<std::uint32_t,5> size{},crc{};};
using SnapshotWire=std::array<std::uint8_t,64>;
SnapshotWire snapshot_encode(const SnapshotManifest& manifest);
bool snapshot_decode(const SnapshotWire& wire,SnapshotManifest& manifest);
int snapshot_select(bool valid_a,const SnapshotManifest& a,bool valid_b,const SnapshotManifest& b);
}

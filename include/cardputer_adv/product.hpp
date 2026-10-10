#pragma once
#include <cstddef>
#include <cstdint>

namespace cardputer_adv {
// Decisões de produto; sem inicialização ou driver nesta etapa.
inline constexpr std::uint16_t kScreenWidth = 240;
inline constexpr std::uint16_t kScreenHeight = 135;
inline constexpr std::uint8_t kSdChipSelect = 12;
inline constexpr std::uint32_t kMenuHideDelayMs = 5000;
inline constexpr std::size_t kChannelSlots = 8; // validar com a base fixada
inline constexpr std::size_t kChannelUrlCapacity = 2048; // inclui terminador
enum class Tab : std::uint8_t { Chat, Nodes, Channels, Settings };
enum class StorageState : std::uint8_t { Missing, Ready, ReadOnly, Full, Error };
enum class ClockState : std::uint8_t { Unknown, GpsSynchronized, Holdover };
} // namespace cardputer_adv

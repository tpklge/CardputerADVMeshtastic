#pragma once
#include "product.hpp"
#include <cstdint>

namespace cardputer_adv {
// Declarações de planejamento; nenhuma função foi implementada.
// Recebe tempo monotônico, independente da hora GPS.
class MenuController {
public:
    void show(std::uint64_t monotonic_ms);
    void hide();
    // Renovar prazo somente se next != active_tab().
    void change_tab(Tab next, std::uint64_t monotonic_ms);
    void tick(std::uint64_t monotonic_ms);
    bool visible() const;
    Tab active_tab() const;
};
} // namespace cardputer_adv

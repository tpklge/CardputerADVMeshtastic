#pragma once
#include <cstdint>

namespace cardputer_adv {
void shared_spi_begin();
void shared_spi_lock();
void shared_spi_unlock();
std::uint32_t shared_spi_max_hold_us();
std::uint32_t shared_spi_max_wait_us();
class SharedSpiGuard {
public:
    SharedSpiGuard() { shared_spi_lock(); }
    ~SharedSpiGuard() { shared_spi_unlock(); }
    SharedSpiGuard(const SharedSpiGuard&) = delete;
    SharedSpiGuard& operator=(const SharedSpiGuard&) = delete;
};
} // namespace cardputer_adv

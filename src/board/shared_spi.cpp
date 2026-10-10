#include "cardputer_adv/shared_spi.hpp"
#include "cardputer_adv/product.hpp"
#include "SPILock.h"
#include "configuration.h"
#include <SPI.h>
#include <atomic>
#include <esp_timer.h>

namespace cardputer_adv {
namespace {
std::atomic<std::uint32_t> max_hold{0};
std::atomic<std::uint32_t> max_wait{0};
std::int64_t acquired_at = 0;
unsigned nesting = 0;
void update_max(std::atomic<std::uint32_t>& value, std::uint32_t candidate) {
    auto current = value.load();
    while (candidate > current && !value.compare_exchange_weak(current, candidate)) {}
}
} // namespace

void shared_spi_begin() {
    static bool started = false;
    if (started) return;
    if (!spiLock) initSPI();
    pinMode(LORA_CS, OUTPUT);
    pinMode(kSdChipSelect, OUTPUT);
    digitalWrite(LORA_CS, HIGH);
    digitalWrite(kSdChipSelect, HIGH);
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    started = true;
}

void shared_spi_lock() {
    const auto before = esp_timer_get_time();
    spiLock->lock();
    const auto after = esp_timer_get_time();
    if (nesting++ == 0) acquired_at = after;
    update_max(max_wait, static_cast<std::uint32_t>(after - before));
}

void shared_spi_unlock() {
    if (--nesting == 0) update_max(max_hold, static_cast<std::uint32_t>(esp_timer_get_time() - acquired_at));
    spiLock->unlock();
}

std::uint32_t shared_spi_max_hold_us() { return max_hold.load(); }
std::uint32_t shared_spi_max_wait_us() { return max_wait.load(); }
} // namespace cardputer_adv

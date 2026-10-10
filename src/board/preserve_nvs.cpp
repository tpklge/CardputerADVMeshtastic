#include "cardputer_adv/board.hpp"
#include <atomic>
#include <cstddef>
#include <esp_partition.h>

namespace {
std::atomic<bool> blocked{false};
}

extern "C" esp_err_t __real_esp_partition_erase_range(const esp_partition_t*, std::size_t, std::size_t);

// Arduino's startup erases NVS after some init errors; preserve launcher data.
extern "C" esp_err_t __wrap_esp_partition_erase_range(const esp_partition_t* partition,
                                                    std::size_t offset, std::size_t size) {
    if (partition && partition->type == ESP_PARTITION_TYPE_DATA &&
        partition->subtype == ESP_PARTITION_SUBTYPE_DATA_NVS) {
        blocked.store(true);
        return ESP_ERR_INVALID_STATE;
    }
    return __real_esp_partition_erase_range(partition, offset, size);
}

namespace cardputer_adv {
bool nvs_erase_was_blocked() { return blocked.load(); }
} // namespace cardputer_adv

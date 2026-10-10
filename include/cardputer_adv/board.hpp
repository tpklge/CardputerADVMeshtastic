#pragma once
#include <cstdint>

namespace cardputer_adv {
struct BoardStatus {
    bool keyboard_ready = false;
    bool cap_ready = false;
    bool gps_time_valid = false;
    bool gps_fix_valid = false;
    bool sd_ready = false;
    bool sd_probe_passed = false;
    bool sd_busy = false;
    bool sd_worker_ready = false;
    bool nvs_erase_blocked = false;
    std::int16_t radio_result = -1;
    std::uint32_t gps_bytes = 0;
    std::uint32_t gps_checksums_failed = 0;
    std::uint32_t radio_packets = 0;
    std::uint32_t radio_errors = 0;
    std::uint32_t sd_errors = 0;
    std::uint32_t sd_chunks = 0;
    std::uint32_t sd_operation_max_us = 0;
    std::uint32_t spi_hold_max_us = 0;
    std::uint32_t spi_wait_max_us = 0;
    std::uint32_t loop_max_us = 0;
    std::uint32_t heap_min_bytes = 0;
    std::uint16_t battery_mv = 0;
    std::uint8_t satellites = 0;
    std::uint8_t hour = 0;
    std::uint8_t minute = 0;
    std::uint8_t second = 0;
    float last_rssi = 0;
    float last_snr = 0;
    char key_label[24] = "--";
    char sd_status[40] = "Inicializando";
};

void board_begin();
void board_tick();
void board_record_loop_time(std::uint32_t microseconds);
bool nvs_erase_was_blocked();
BoardStatus board_snapshot();
bool board_request_sd_probe();
void diagnostics_begin();
void diagnostics_tick();
} // namespace cardputer_adv

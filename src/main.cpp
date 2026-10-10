#include <Arduino.h>
#include <esp_timer.h>
#include "cardputer_adv/board.hpp"

// M1 runs hardware diagnostics; the mesh runtime will take ownership in M2.
void setup() {
    cardputer_adv::board_begin();
    cardputer_adv::diagnostics_begin();
}

void loop() {
    const auto started = esp_timer_get_time();
    cardputer_adv::board_tick();
    cardputer_adv::diagnostics_tick();
    delay(1);
    cardputer_adv::board_record_loop_time(static_cast<std::uint32_t>(esp_timer_get_time() - started));
}

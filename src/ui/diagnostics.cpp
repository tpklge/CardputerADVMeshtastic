#include "cardputer_adv/board.hpp"
#include "configuration.h"
#include <ST7789Spi.h>
#include <OLEDDisplayFonts.h>
#include <cstdio>

namespace cardputer_adv {
namespace {
SPIClass display_spi(HSPI);
ST7789Spi display(&display_spi, ST7789_RESET, ST7789_RS, ST7789_NSS,
                 GEOMETRY_RAWMODE, TFT_WIDTH, TFT_HEIGHT,
                 ST7789_SDA, ST7789_MISO, ST7789_SCK);
std::uint32_t last_frame = 0;
bool display_ready = false;
void line(int y, const char* text) { display.drawString(2, y, text); }
} // namespace

void diagnostics_begin() {
    pinMode(VTFT_LEDA, OUTPUT);
    digitalWrite(VTFT_LEDA, HIGH);
    display_ready = display.init();
    if (!display_ready) return;
    // The panel is mounted opposite to the driver's default landscape orientation.
    display.flipScreenVertically();
    display.setRGB(ST77XX_WHITE);
    display.setFont(ArialMT_Plain_10);
}

void diagnostics_tick() {
    const auto now = millis();
    if (!display_ready || static_cast<std::uint32_t>(now - last_frame) < 250) return;
    last_frame = now;
    const auto s = board_snapshot();
    char text[80];
    display.clear();
    line(0, "Cardputer ADV | M1 hardware");
    std::snprintf(text, sizeof(text), "Cap %s  SX1262 %d | RX 915MHz", s.cap_ready ? "OK" : "--", s.radio_result);
    line(13, text);
    std::snprintf(text, sizeof(text), "RX %lu / erros %lu | bat %.2fV", static_cast<unsigned long>(s.radio_packets),
                  static_cast<unsigned long>(s.radio_errors), s.battery_mv / 1000.f);
    line(26, text);
    if (s.gps_time_valid) std::snprintf(text, sizeof(text), "GPS UTC %02u:%02u:%02u  fix %s  sat %u",
                                      s.hour, s.minute, s.second, s.gps_fix_valid ? "OK" : "--", s.satellites);
    else std::snprintf(text, sizeof(text), "GPS --:-- | NMEA %lu / CRC %lu", static_cast<unsigned long>(s.gps_bytes),
                       static_cast<unsigned long>(s.gps_checksums_failed));
    line(39, text);
    std::snprintf(text, sizeof(text), "SD: %s", s.sd_busy ? "Testando..." : s.sd_status);
    line(52, text);
    std::snprintf(text, sizeof(text), "SPI max %lu us / espera %lu us", static_cast<unsigned long>(s.spi_hold_max_us),
                  static_cast<unsigned long>(s.spi_wait_max_us));
    line(65, text);
    std::snprintf(text, sizeof(text), "SD blocos %lu / erros %lu", static_cast<unsigned long>(s.sd_chunks),
                  static_cast<unsigned long>(s.sd_errors));
    line(78, text);
    std::snprintf(text, sizeof(text), "Teclado %s: %s", s.keyboard_ready ? "OK" : "--", s.key_label);
    line(91, text);
    std::snprintf(text, sizeof(text), "Heap min %lu | loop max %lu us", static_cast<unsigned long>(s.heap_min_bytes),
                  static_cast<unsigned long>(s.loop_max_us));
    line(104, text);
    line(120, s.nvs_erase_blocked ? "NVS preservado | Enter: teste SD" : "Enter: teste SD | sem TX / sem mesh");
    display.display();
}
} // namespace cardputer_adv

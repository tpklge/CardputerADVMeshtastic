#include "cardputer_adv/board.hpp"
#include "cardputer_adv/keyboard_decoder.hpp"
#include "cardputer_adv/product.hpp"
#include "cardputer_adv/shared_spi.hpp"
#include "configuration.h"
#include "input/TCA8418KeyboardBase.h"
#include <RadioLib.h>
#include <SD.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <esp_heap_caps.h>
#include <esp_timer.h>

namespace cardputer_adv {
namespace {
BoardStatus status;
portMUX_TYPE status_mux = portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t sd_task = nullptr;
HardwareSerial gps_serial(1);
TinyGPSPlus gps_parser;
KeyboardDecoder decoder;
constexpr auto probe_path = "/CardputerADVMeshtastic/diagnostics/m1-spi-probe.bin";
constexpr auto probe_size = 4096u;
std::uint32_t battery_at = 0;
std::uint32_t keyboard_at = 0;
volatile bool radio_irq = false;

class SharedRadioHal : public ArduinoHal {
public:
    SharedRadioHal() : ArduinoHal(SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0)) {}
    void spiBeginTransaction() override {
        shared_spi_lock();
        ArduinoHal::spiBeginTransaction();
    }
    void spiEndTransaction() override {
        ArduinoHal::spiEndTransaction();
        shared_spi_unlock();
    }
};
SharedRadioHal radio_hal;
Module radio_module(&radio_hal, SX126X_CS, SX126X_DIO1, SX126X_RESET, SX126X_BUSY);
SX1262 radio(&radio_module);

class DiagnosticKeyboard : public TCA8418KeyboardBase {
public:
    DiagnosticKeyboard() : TCA8418KeyboardBase(7, 8) {}
    std::uint8_t count() { return keyCount(); }
    std::uint8_t pop() { return readRegister(TCA8418_REG_KEY_EVENT_A); }
    bool overflowed() { return (readRegister(TCA8418_REG_INT_STAT) & 0x08) != 0; }
    void recover() {
        // Bound FIFO draining even if the keyboard disconnects during recovery.
        for (int i = 0; i < 10; ++i) {
            if (!pop()) break;
        }
        writeRegister(TCA8418_REG_INT_STAT, 0x1f);
    }
};
DiagnosticKeyboard keyboard;

bool probe_i2c(std::uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

bool write_i2c(std::uint8_t address, std::uint8_t reg, std::uint8_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

void sd_message(const char* text, bool ready, bool failed = false) {
    portENTER_CRITICAL(&status_mux);
    std::snprintf(status.sd_status, sizeof(status.sd_status), "%s", text);
    status.sd_ready = ready;
    if (failed) ++status.sd_errors;
    portEXIT_CRITICAL(&status_mux);
}

bool mount_sd() {
    SharedSpiGuard guard;
    // format_if_empty is false; mounting does not reinitialize SPI.
    return SD.begin(kSdChipSelect, SPI, 4000000, "/sd", 4, false) && SD.cardType() != CARD_NONE;
}

bool ensure_diagnostic_directory() {
    SharedSpiGuard guard;
    const char* paths[] = {"/CardputerADVMeshtastic", "/CardputerADVMeshtastic/diagnostics"};
    for (const auto* path : paths) {
        if (!SD.exists(path) && !SD.mkdir(path)) return false;
    }
    return true;
}

void record_sd_chunk(std::uint32_t elapsed) {
    portENTER_CRITICAL(&status_mux);
    ++status.sd_chunks;
    status.sd_operation_max_us = std::max(status.sd_operation_max_us, elapsed);
    portEXIT_CRITICAL(&status_mux);
}

bool run_sd_probe() {
    if (!ensure_diagnostic_directory()) return false;
    File file;
    {
        SharedSpiGuard guard;
        file = SD.open(probe_path, FILE_WRITE);
    }
    if (!file) return false;
    bool okay = true;
    std::uint8_t block[256];
    for (std::uint32_t offset = 0; offset < probe_size && okay; offset += sizeof(block)) {
        for (std::size_t i = 0; i < sizeof(block); ++i) block[i] = (offset + i) * 37u + 17u;
        const auto started = esp_timer_get_time();
        {
            SharedSpiGuard guard;
            okay = file.write(block, sizeof(block)) == sizeof(block);
        }
        record_sd_chunk(static_cast<std::uint32_t>(esp_timer_get_time() - started));
        vTaskDelay(1);
    }
    {
        SharedSpiGuard guard;
        file.flush();
        file.close();
        file = SD.open(probe_path, FILE_READ);
        okay = okay && file && file.size() == probe_size;
    }
    for (std::uint32_t offset = 0; offset < probe_size && okay; offset += sizeof(block)) {
        const auto started = esp_timer_get_time();
        {
            SharedSpiGuard guard;
            okay = file.read(block, sizeof(block)) == sizeof(block);
        }
        if (okay) {
            for (std::size_t i = 0; i < sizeof(block); ++i) {
                if (block[i] != static_cast<std::uint8_t>((offset + i) * 37u + 17u)) {
                    okay = false;
                    break;
                }
            }
        }
        record_sd_chunk(static_cast<std::uint32_t>(esp_timer_get_time() - started));
        vTaskDelay(1);
    }
    {
        SharedSpiGuard guard;
        file.close();
    }
    return okay;
}

bool save_report() {
    const auto snapshot = board_snapshot();
    char report[512];
    const int length = std::snprintf(report, sizeof(report),
        "{\"stage\":\"M1\",\"tx_enabled\":false,\"sd_probe_ok\":%s,"
        "\"radio_result\":%d,\"rx_packets\":%lu,\"radio_errors\":%lu,"
        "\"gps_bytes\":%lu,\"gps_crc_errors\":%lu,\"sd_chunks\":%lu,"
        "\"sd_errors\":%lu,\"spi_hold_max_us\":%lu,\"spi_wait_max_us\":%lu,"
        "\"heap_min_bytes\":%lu,\"loop_max_us\":%lu}\n",
        snapshot.sd_probe_passed ? "true" : "false", snapshot.radio_result,
        static_cast<unsigned long>(snapshot.radio_packets), static_cast<unsigned long>(snapshot.radio_errors),
        static_cast<unsigned long>(snapshot.gps_bytes), static_cast<unsigned long>(snapshot.gps_checksums_failed),
        static_cast<unsigned long>(snapshot.sd_chunks), static_cast<unsigned long>(snapshot.sd_errors),
        static_cast<unsigned long>(snapshot.spi_hold_max_us), static_cast<unsigned long>(snapshot.spi_wait_max_us),
        static_cast<unsigned long>(snapshot.heap_min_bytes), static_cast<unsigned long>(snapshot.loop_max_us));
    if (length <= 0 || static_cast<std::size_t>(length) >= sizeof(report)) return false;
    File file;
    {
        SharedSpiGuard guard;
        file = SD.open("/CardputerADVMeshtastic/diagnostics/m1-report.json", FILE_WRITE);
    }
    if (!file) return false;
    bool okay = true;
    for (int offset = 0; offset < length && okay; offset += 256) {
        const auto chunk = std::min(256, length - offset);
        {
            SharedSpiGuard guard;
            okay = file.write(reinterpret_cast<const std::uint8_t*>(report + offset), chunk) ==
                   static_cast<std::size_t>(chunk);
        }
        vTaskDelay(1);
    }
    {
        SharedSpiGuard guard;
        file.flush();
        file.close();
    }
    return okay;
}

void sd_worker(void*) {
    const bool mounted = mount_sd();
    sd_message(mounted ? "Montado; Enter testa" : "SD ausente/erro; Enter tenta", mounted);
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        portENTER_CRITICAL(&status_mux);
        status.sd_busy = true;
        status.sd_probe_passed = false;
        portEXIT_CRITICAL(&status_mux);
        portENTER_CRITICAL(&status_mux);
        const bool already_mounted = status.sd_ready;
        portEXIT_CRITICAL(&status_mux);
        if (!already_mounted) {
            SharedSpiGuard guard;
            SD.end();
        }
        const bool ready = already_mounted || mount_sd();
        const bool okay = ready && run_sd_probe();
        sd_message(okay ? "Leitura/escrita 4KiB OK" : "Falha SD; Enter tenta", ready && okay, !okay);
        portENTER_CRITICAL(&status_mux);
        status.sd_probe_passed = okay;
        portEXIT_CRITICAL(&status_mux);
        if (okay && !save_report()) sd_message("Teste OK; erro ao salvar relatorio", false, true);
        portENTER_CRITICAL(&status_mux);
        status.sd_busy = false;
        portEXIT_CRITICAL(&status_mux);
    }
}

void IRAM_ATTR radio_interrupt() { radio_irq = true; }

void publish_key(KeyEvent event) {
    if (event.action == KeyAction::None) return;
    char label[24];
    const char* name = "";
    switch (event.action) {
    case KeyAction::Text: std::snprintf(label, sizeof(label), "'%c'%s", event.character, event.repeat ? " rep" : ""); break;
    case KeyAction::Tab: name = "Tab"; break;
    case KeyAction::BackTab: name = "Shift+Tab"; break;
    case KeyAction::Enter: name = "Enter"; break;
    case KeyAction::Send: name = "Fn+Enter"; break;
    case KeyAction::Backspace: name = "Backspace"; break;
    case KeyAction::Escape: name = "Fn+Esc"; break;
    case KeyAction::Left: name = "Fn+Esquerda"; break;
    case KeyAction::Right: name = "Fn+Direita"; break;
    case KeyAction::Up: name = "Fn+Cima"; break;
    case KeyAction::Down: name = "Fn+Baixo"; break;
    default: break;
    }
    if (event.action != KeyAction::Text) std::snprintf(label, sizeof(label), "%s%s", name, event.repeat ? " rep" : "");
    portENTER_CRITICAL(&status_mux);
    std::snprintf(status.key_label, sizeof(status.key_label), "%s", label);
    portEXIT_CRITICAL(&status_mux);
    if (event.action == KeyAction::Enter && !event.repeat) board_request_sd_probe();
}
} // namespace

void board_begin() {
    shared_spi_begin();
    status.nvs_erase_blocked = nvs_erase_was_blocked();
    Wire.begin(I2C_SDA, I2C_SCL, 400000);
    Wire.setTimeOut(20);
    status.keyboard_ready = probe_i2c(0x34);
    if (status.keyboard_ready) keyboard.begin(0x34, &Wire);
    status.cap_ready = probe_i2c(0x43) && write_i2c(0x43, 0x03, 0x01) &&
                       write_i2c(0x43, 0x07, 0x00) && write_i2c(0x43, 0x05, 0x01);
    gps_serial.setRxBufferSize(2048);
    gps_serial.begin(GPS_BAUDRATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    pinMode(BATTERY_PIN, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(BATTERY_PIN, ADC_11db);
    if (status.cap_ready) {
        // Receive-only hardware bench profile, not a Meshtastic network configuration.
        status.radio_result = radio.begin(915.0, 250.0, 11, 5, 0x2b, 0, 16, SX126X_DIO3_TCXO_VOLTAGE);
        if (status.radio_result == RADIOLIB_ERR_NONE) {
            status.radio_result = radio.setDio2AsRfSwitch(true);
            if (status.radio_result == RADIOLIB_ERR_NONE) {
                radio.setDio1Action(radio_interrupt);
                status.radio_result = radio.startReceive();
            }
        }
    }
    const bool worker_ready = xTaskCreate(sd_worker, "adv-sd", 4096, nullptr, 1, &sd_task) == pdPASS;
    portENTER_CRITICAL(&status_mux);
    status.sd_worker_ready = worker_ready;
    portEXIT_CRITICAL(&status_mux);
    if (!worker_ready) sd_message("Erro ao iniciar worker SD", false, true);
}

bool board_request_sd_probe() {
    portENTER_CRITICAL(&status_mux);
    const bool accepted = sd_task && !status.sd_busy;
    if (accepted) status.sd_busy = true;
    portEXIT_CRITICAL(&status_mux);
    if (accepted) xTaskNotifyGive(sd_task);
    return accepted;
}

void board_tick() {
    const auto now = millis();
    for (unsigned i = 0; i < 256 && gps_serial.available(); ++i) {
        gps_parser.encode(gps_serial.read());
    }
    if (status.keyboard_ready && static_cast<std::uint32_t>(now - keyboard_at) >= 10) {
        keyboard_at = now;
        if (keyboard.overflowed()) {
            keyboard.recover();
            decoder.reset();
        }
        const auto count = std::min<unsigned>(keyboard.count(), 10);
        for (unsigned i = 0; i < count; ++i) publish_key(decoder.feed(keyboard.pop(), now));
        keyboard.clearInt();
        publish_key(decoder.tick(now));
    }
    if (radio_irq && status.radio_result == RADIOLIB_ERR_NONE) {
        radio_irq = false;
        std::uint8_t packet[256];
        const auto length = std::min<std::size_t>(radio.getPacketLength(), sizeof(packet));
        const auto result = radio.readData(packet, length);
        const auto rssi = result == RADIOLIB_ERR_NONE ? radio.getRSSI() : 0;
        const auto snr = result == RADIOLIB_ERR_NONE ? radio.getSNR() : 0;
        const auto receive_result = radio.startReceive();
        portENTER_CRITICAL(&status_mux);
        if (result == RADIOLIB_ERR_NONE) {
            ++status.radio_packets;
            status.last_rssi = rssi;
            status.last_snr = snr;
        } else ++status.radio_errors;
        status.radio_result = receive_result;
        portEXIT_CRITICAL(&status_mux);
    }
    if (static_cast<std::uint32_t>(now - battery_at) >= 1000) {
        battery_at = now;
        const auto millivolts = static_cast<std::uint16_t>(analogReadMilliVolts(BATTERY_PIN) * 2.04f);
        portENTER_CRITICAL(&status_mux);
        status.battery_mv = millivolts;
        portEXIT_CRITICAL(&status_mux);
    }
    const auto heap_min = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    portENTER_CRITICAL(&status_mux);
    status.gps_bytes = gps_parser.charsProcessed();
    status.gps_checksums_failed = gps_parser.failedChecksum();
    status.gps_time_valid = gps_parser.time.isValid() && gps_parser.time.age() < 5000;
    status.gps_fix_valid = gps_parser.location.isValid() && gps_parser.location.age() < 5000;
    status.hour = gps_parser.time.hour();
    status.minute = gps_parser.time.minute();
    status.second = gps_parser.time.second();
    status.satellites = gps_parser.satellites.isValid() ? std::min<unsigned>(gps_parser.satellites.value(), 255) : 0;
    status.heap_min_bytes = heap_min;
    status.spi_hold_max_us = shared_spi_max_hold_us();
    status.spi_wait_max_us = shared_spi_max_wait_us();
    portEXIT_CRITICAL(&status_mux);
}

void board_record_loop_time(std::uint32_t microseconds) {
    portENTER_CRITICAL(&status_mux);
    status.loop_max_us = std::max(status.loop_max_us, microseconds);
    portEXIT_CRITICAL(&status_mux);
}

BoardStatus board_snapshot() {
    portENTER_CRITICAL(&status_mux);
    const auto snapshot = status;
    portEXIT_CRITICAL(&status_mux);
    return snapshot;
}
} // namespace cardputer_adv

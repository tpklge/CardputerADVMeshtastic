#ifdef CARDPUTER_ADV_M2
#include <Arduino.h>
#include <Wire.h>
#include "configuration.h"
#include "cardputer_adv/shared_spi.hpp"
#include "cardputer_adv/core_storage.hpp"
#include "cardputer_adv/mesh_backend.hpp"
#include "cardputer_adv/m2_console.hpp"
#include "input/TCA8418KeyboardBase.h"
#include "mesh/NodeDB.h"
#include "mesh/RadioLibInterface.h"
#include "Throttle.h"
#ifdef CARDPUTER_ADV_M3
#include "cardputer_adv/app_storage.hpp"
#endif

void meshtastic_setup();
void meshtastic_loop();
namespace {
class Keyboard : public TCA8418KeyboardBase {
public:
    Keyboard():TCA8418KeyboardBase(7,8) {}
    uint8_t count() {return keyCount();}
    uint8_t pop() {return readRegister(TCA8418_REG_KEY_EVENT_A);}
    bool overflow() {return (readRegister(TCA8418_REG_INT_STAT)&8)!=0;}
    void recover() {for(int i=0;i<10;++i)if(!pop())break;writeRegister(TCA8418_REG_INT_STAT,0x1f);}
} keyboard;
cardputer_adv::KeyboardDecoder decoder;
bool keyboard_ready=false, core_running=false;
#ifdef CARDPUTER_ADV_M3
bool sd_failure_reported=false;
#endif
std::uint32_t key_at=0;
bool write_reg(uint8_t reg,uint8_t value) {Wire.beginTransmission(0x43);Wire.write(reg);Wire.write(value);return Wire.endTransmission()==0;}
}
void setup() {
    using namespace cardputer_adv;
    shared_spi_begin();
    Wire.begin(I2C_SDA,I2C_SCL,400000);Wire.setTimeOut(20);
    m2_console_begin();m2_console_message("Iniciando nucleo M2...");m2_console_tick();
    Wire.beginTransmission(0x43);bool cap=Wire.endTransmission()==0;
    cap=cap&&write_reg(0x03,1)&&write_reg(0x07,0)&&write_reg(0x05,1);
    if(!core_storage_mount()) m2_console_message(core_storage_fault()?"Config SD incompleta: nucleo parado":"SD ausente/falha: nucleo parado");
    else if(!cap) m2_console_message("Cap ausente: nucleo parado");
    else {
        meshtastic_setup();
        if(!core_storage_fault()) {
            // Force readback-backed creation of the basic node configuration on SD.
            core_running=nodeDB&&nodeDB->saveToDisk();
            if(!core_running) core_storage_set_fault();
        }
#ifdef CARDPUTER_ADV_M3
        if(core_running && !app_storage_begin())m2_console_message("Historico SD falhou; TX bloqueado");
#endif
        mesh_backend_begin();
        #ifdef CARDPUTER_ADV_M3
        m2_console_message(core_running?(app_storage_status().ready?
#ifdef CARDPUTER_ADV_M8
        "M8 pronto. Temas nos AJUSTES"
#elif defined(CARDPUTER_ADV_M7_NODES)
        "M7: busca e favoritos nos NOS"
#elif defined(CARDPUTER_ADV_M7)
        "M7 pronto. DM via NodeInfo PKI"
#elif defined(CARDPUTER_ADV_M6)
        "M6 pronto. Canais editaveis"
#elif defined(CARDPUTER_ADV_M5)
        "M5 pronto. Tab: menu"
#elif defined(CARDPUTER_ADV_M4)
        "M4 pronto. Tab: menu"
#else
        "M3 pronto. Tab: historico / ajuda"
#endif
:"Historico SD falhou; TX bloqueado"):"Config SD invalida: nucleo parado");
#else
        m2_console_message(core_running?"M2 pronto. Tab: ajuda / nos":"Config SD invalida: nucleo parado");
#endif
    }
#ifdef CARDPUTER_ADV_M4
    m2_console_tick(); // Restore the UI before accepting the first keyboard event.
#endif
    Wire.beginTransmission(0x34);keyboard_ready=Wire.endTransmission()==0;
    if(keyboard_ready)keyboard.begin(0x34,&Wire);
}
void loop() {
    using namespace cardputer_adv;
#ifdef CARDPUTER_ADV_M8
    auto loop_start=millis();
#endif
    if(core_running) core_storage_health_tick();
#ifdef CARDPUTER_ADV_M3
    if(core_running) {
        if(core_storage_fault() && !sd_failure_reported) {sd_failure_reported=true;m2_console_message("SD erro: RX nesta sessao; TX bloqueado");}
        meshtastic_loop();mesh_backend_tick();
    }
#else
    if(core_running&&!core_storage_fault()) {meshtastic_loop();mesh_backend_tick();}
    else if(core_running) {core_running=false;config.lora.tx_enabled=false;
        if(RadioLibInterface::instance)RadioLibInterface::instance->RadioInterface::disable();
        m2_console_message("Falha SD: reinicie com cartao valido");}
#endif
    if(keyboard_ready&&!Throttle::isWithinTimespanMs(key_at,10)) {
        key_at=millis();if(keyboard.overflow()){keyboard.recover();decoder.reset();}
        auto n=keyboard.count();if(n>10)n=10;
        for(unsigned i=0;i<n;++i)m2_console_key(decoder.feed(keyboard.pop(),key_at));
        keyboard.clearInt();m2_console_key(decoder.tick(key_at));
    }
    m2_console_tick();
#ifdef CARDPUTER_ADV_M8
    mesh_observe_loop(std::uint32_t(millis()-loop_start));
#endif
    delay(1);
}
#endif

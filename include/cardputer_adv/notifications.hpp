#pragma once
#include <cstdint>
namespace cardputer_adv {
class ScreenActivity {
    std::uint32_t active_at_=0;
    bool lit_=true;
public:
    static constexpr std::uint32_t timeout_ms=30000;
    void begin(std::uint32_t now){active_at_=now;lit_=true;}
    bool wake(std::uint32_t now){active_at_=now;bool changed=!lit_;lit_=true;return changed;}
    bool tick(std::uint32_t now){if(lit_ && std::uint32_t(now-active_at_)>=timeout_ms){lit_=false;return true;}return false;}
    bool lit() const{return lit_;}
};
bool notification_audio_begin();
void notification_audio_beep();
}

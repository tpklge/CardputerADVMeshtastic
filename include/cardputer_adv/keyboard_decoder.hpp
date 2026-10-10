#pragma once
#include <array>
#include <cstdint>

namespace cardputer_adv {
enum class KeyAction : std::uint8_t {
    None, Text, Tab, BackTab, Enter, Send, Backspace, Escape, Left, Right, Up, Down
};
struct KeyEvent {
    KeyAction action = KeyAction::None;
    char character = 0;
    bool repeat = false;
};
class KeyboardDecoder {
public:
    KeyEvent feed(std::uint8_t fifo_event, std::uint32_t now_ms);
    KeyEvent tick(std::uint32_t now_ms);
    void reset();
private:
    KeyEvent decode(std::uint8_t index) const;
    std::array<bool, 56> held_{};
    std::uint8_t repeat_key_ = 255;
    std::uint32_t repeat_at_ = 0;
    bool repeating_ = false;
};
} // namespace cardputer_adv

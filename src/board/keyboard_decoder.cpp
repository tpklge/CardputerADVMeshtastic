#include "cardputer_adv/keyboard_decoder.hpp"

namespace cardputer_adv {
namespace {
// Physical matrix order follows the pinned Cardputer ADV variant.
constexpr std::array<char, 56> normal = {
    '`', '\t', 0, 0, '1', 'q', 0, 0, '2', 'w', 'a', 0, '3', 'e', 's', 'z',
    '4', 'r', 'd', 'x', '5', 't', 'f', 'c', '6', 'y', 'g', 'v', '7', 'u', 'h', 'b',
    '8', 'i', 'j', 'n', '9', 'o', 'k', 'm', '0', 'p', 'l', ',', '-', '[', ';', '.',
    '=', ']', '\'', '/', '\b', '\\', '\r', ' '
};
constexpr std::array<char, 56> shifted = {
    '~', '\t', 0, 0, '!', 'Q', 0, 0, '@', 'W', 'A', 0, '#', 'E', 'S', 'Z',
    '$', 'R', 'D', 'X', '%', 'T', 'F', 'C', '^', 'Y', 'G', 'V', '&', 'U', 'H', 'B',
    '*', 'I', 'J', 'N', '(', 'O', 'K', 'M', ')', 'P', 'L', '<', '_', '{', ':', '>',
    '+', '}', '"', '?', '\b', '|', '\r', ' '
};
constexpr std::uint8_t fn = 2;
constexpr std::uint8_t shift = 6;
bool repeatable(KeyAction action) {
    return action == KeyAction::Text || action == KeyAction::Backspace ||
           action == KeyAction::Left || action == KeyAction::Right ||
           action == KeyAction::Up || action == KeyAction::Down;
}
} // namespace

KeyEvent KeyboardDecoder::decode(std::uint8_t index) const {
#ifdef CARDPUTER_ADV_UI_V2
    if(!held_[shift]){
        if(index==54 && held_[fn])return {KeyAction::Send};
        if(!held_[fn])switch(index){case 0:return {KeyAction::Escape};case 43:return {KeyAction::Left};case 46:return {KeyAction::Up};case 47:return {KeyAction::Down};case 51:return {KeyAction::Right};default:break;}
    }
#else
    if (held_[fn]) {
        switch (index) {
        case 0: return {KeyAction::Escape};
        case 43: return {KeyAction::Left};
        case 46: return {KeyAction::Up};
        case 47: return {KeyAction::Down};
        case 51: return {KeyAction::Right};
        case 54: return {KeyAction::Send};
        default: break;
        }
    }
#endif
    if (index == 1) return {held_[shift] ? KeyAction::BackTab : KeyAction::Tab};
    if (index == 52) return {KeyAction::Backspace};
    if (index == 54) return {KeyAction::Enter};
    const char c = held_[shift] ? shifted[index] : normal[index];
    return c ? KeyEvent{KeyAction::Text, c} : KeyEvent{};
}

KeyEvent KeyboardDecoder::feed(std::uint8_t fifo_event, std::uint32_t now_ms) {
    const auto key = static_cast<std::uint8_t>(fifo_event & 0x7f);
    if (key == 0) return {};
    const auto row = static_cast<std::uint8_t>((key - 1) / 10);
    const auto column = static_cast<std::uint8_t>((key - 1) % 10);
    if (row >= 7 || column >= 8) return {};
    const auto index = static_cast<std::uint8_t>(row * 8 + column);
    const bool pressed = (fifo_event & 0x80) != 0;
    if (!pressed) {
        held_[index] = false;
        if (repeat_key_ == index) repeat_key_ = 255;
        return {};
    }
    if (held_[index]) return {};
    held_[index] = true;
    const auto event = decode(index);
    if (repeatable(event.action)) {
        repeat_key_ = index;
        repeat_at_ = now_ms;
        repeating_ = false;
    } else if (event.action != KeyAction::None) {
        repeat_key_ = 255;
    }
    return event;
}

KeyEvent KeyboardDecoder::tick(std::uint32_t now_ms) {
    if (repeat_key_ == 255 || !held_[repeat_key_]) return {};
    const auto interval = repeating_ ? 80u : 450u;
    if (static_cast<std::uint32_t>(now_ms - repeat_at_) < interval) return {};
    repeat_at_ = now_ms;
    repeating_ = true;
    auto event = decode(repeat_key_);
    if (!repeatable(event.action)) return {};
    event.repeat = true;
    return event;
}

void KeyboardDecoder::reset() {
    held_.fill(false);
    repeat_key_ = 255;
    repeating_ = false;
}
} // namespace cardputer_adv

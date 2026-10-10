#include "cardputer_adv/keyboard_decoder.hpp"
#include <cassert>
#include <cstdint>
#include <limits>

using namespace cardputer_adv;
constexpr std::uint8_t fifo(unsigned index, bool down) {
    return static_cast<std::uint8_t>(1 + (index / 8) * 10 + index % 8 + (down ? 0x80 : 0));
}

int main() {
    KeyboardDecoder decoder;
    assert(decoder.feed(0, 0).action == KeyAction::None);
    assert(decoder.feed(0xff, 0).action == KeyAction::None);
    assert(decoder.feed(0x89, 0).action == KeyAction::None);

    decoder.feed(fifo(6, true), 1);
    assert(decoder.feed(fifo(1, true), 2).action == KeyAction::BackTab);
    decoder.feed(fifo(1, false), 3);
    assert(decoder.feed(fifo(13, true), 4).character == 'E');
    decoder.feed(fifo(6, false), 5);
    decoder.feed(fifo(13, false), 6);
    assert(decoder.feed(fifo(13, true), 7).character == 'e');
    decoder.reset();

    decoder.feed(fifo(2, true), 10);
    assert(decoder.feed(fifo(54, true), 11).action == KeyAction::Send);
    assert(decoder.tick(1000).action == KeyAction::None);
    decoder.feed(fifo(54, false), 12);
    assert(decoder.feed(fifo(51, true), 13).action == KeyAction::Right);
    decoder.feed(fifo(2, false), 14);
    assert(decoder.tick(500).character == '/');
    decoder.feed(fifo(51, false), 501);
    assert(decoder.tick(1000).action == KeyAction::None);
    decoder.reset();

    const char* link = "https://meshtastic.org/e/?add=true#Aa-_=123";
    for (const char* p = link; *p; ++p) {
        bool produced = false;
        for (unsigned index = 0; index < 56 && !produced; ++index) {
            for (unsigned shifted = 0; shifted < 2 && !produced; ++shifted) {
                decoder.reset();
                if (shifted) decoder.feed(fifo(6, true), 0);
                const auto event = decoder.feed(fifo(index, true), 1);
                produced = event.action == KeyAction::Text && event.character == *p;
            }
        }
        assert(produced);
    }

    decoder.reset();
    const auto start = std::numeric_limits<std::uint32_t>::max() - 100;
    assert(decoder.feed(fifo(9, true), start).character == 'w');
    assert(decoder.feed(fifo(9, true), start + 1).action == KeyAction::None);
    assert(decoder.tick(start + 449).action == KeyAction::None);
    assert(decoder.tick(start + 450).repeat);
    assert(decoder.tick(start + 529).action == KeyAction::None);
    assert(decoder.tick(start + 530).repeat);
    decoder.feed(fifo(9, false), start + 531);
    assert(decoder.tick(start + 1000).action == KeyAction::None);

    decoder.reset();
    assert(decoder.feed(fifo(1, true), 0).action == KeyAction::Tab);
    assert(decoder.tick(1000).action == KeyAction::None);
    decoder.reset();
    decoder.feed(fifo(2, true), 0);
    assert(decoder.feed(fifo(31, true), 1).character == 'b');
}

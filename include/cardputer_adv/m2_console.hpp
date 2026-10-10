#pragma once
#include "keyboard_decoder.hpp"
namespace cardputer_adv {
void m2_console_begin();
void m2_console_tick();
void m2_console_key(const KeyEvent& event);
void m2_console_message(const char* text);
}

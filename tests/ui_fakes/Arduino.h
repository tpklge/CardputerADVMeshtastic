#pragma once
#include <cstdint>
#define PROGMEM
constexpr int HSPI=2,OUTPUT=1,HIGH=1;
inline std::uint32_t fake_millis=0;
inline std::uint32_t millis(){return fake_millis;}
inline void pinMode(int,int){}
inline int fake_backlight=HIGH;
inline void digitalWrite(int,int value){fake_backlight=value;}
struct SPIClass {explicit SPIClass(int){}};

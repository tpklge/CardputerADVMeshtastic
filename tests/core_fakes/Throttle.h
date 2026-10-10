#pragma once
struct Throttle {static bool isWithinTimespanMs(std::uint32_t at,unsigned span){return std::uint32_t(millis()-at)<span;}};

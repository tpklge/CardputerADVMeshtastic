#pragma once
#include <cstdint>
constexpr int RTCQualityGPS=1;
inline std::uint32_t fake_storage_utc=1800000000;
inline std::uint32_t getValidTime(int){return fake_storage_utc;}

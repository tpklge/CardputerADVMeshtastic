#pragma once
#include <cstring>
constexpr int ESP_MAC_WIFI_STA=0,ESP_OK=0;
inline int esp_efuse_mac_get_default(unsigned char* out){const unsigned char mac[6]={1,2,3,4,5,6};std::memcpy(out,mac,6);return ESP_OK;}

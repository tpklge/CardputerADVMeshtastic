#pragma once
#include <cstddef>
#include <cstring>
inline bool fake_restarted=false;
inline void esp_restart(){fake_restarted=true;}
inline void esp_fill_random(void* out,std::size_t count){std::memset(out,42,count);}

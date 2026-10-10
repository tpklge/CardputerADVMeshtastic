#pragma once
#include <string>
#include <cstdint>
#include <algorithm>
class String:public std::string {
public:
    using std::string::string;
    String(const std::string& s):std::string(s){}
    explicit String(unsigned n):std::string(std::to_string(n)){}
    bool isEmpty()const{return empty();}
    bool startsWith(const char* s)const{return rfind(s,0)==0;}
    int indexOf(const char* s)const{auto n=find(s);return n==npos?-1:int(n);}
};
inline std::uint32_t fake_millis=3000;
inline std::uint32_t millis(){return fake_millis;}
using SemaphoreHandle_t=void*;
constexpr int portMAX_DELAY=0;
inline SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(){return reinterpret_cast<void*>(1);}
inline void xSemaphoreTakeRecursive(SemaphoreHandle_t,int){}
inline void xSemaphoreGiveRecursive(SemaphoreHandle_t){}
inline int SPI=0;

#pragma once
#include <cstdint>
#include <string>
#include <algorithm>
#include <stdexcept>
using String=std::string;
inline std::uint32_t fake_storage_millis=1000;
inline std::uint32_t millis(){return fake_storage_millis;}
using portMUX_TYPE=int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
struct FakeSemaphore{int value;};
using SemaphoreHandle_t=FakeSemaphore*;
constexpr int pdTRUE=1,pdPASS=1;
inline unsigned pdMS_TO_TICKS(unsigned x){return x;}
inline SemaphoreHandle_t xSemaphoreCreateBinary(){static FakeSemaphore b{0};return &b;}
inline SemaphoreHandle_t xSemaphoreCreateMutex(){static FakeSemaphore m{1};return &m;}
inline int xSemaphoreTake(SemaphoreHandle_t s,unsigned){if(!s || !s->value)return 0;--s->value;return 1;}
inline void xSemaphoreGive(SemaphoreHandle_t s){++s->value;}
inline void (*fake_worker)(void*)=nullptr;
inline int xTaskCreate(void (*f)(void*),const char*,unsigned,void*,unsigned,void*){fake_worker=f;return 1;}
struct WorkerYield{};
inline void vTaskDelay(unsigned){throw WorkerYield{};}
inline void storage_step(){++fake_storage_millis;try{fake_worker(nullptr);}catch(const WorkerYield&) {}}

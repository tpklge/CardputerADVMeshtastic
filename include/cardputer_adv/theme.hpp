#pragma once
#include <cstdint>
namespace cardputer_adv {
struct Theme {const char* name;std::uint16_t background,text,panel,sent,received,active,active_text;};
constexpr Theme themes[]={
    {"Escuro",0x0841,0xffff,0x1924,0x67f2,0x65ff,0x65ff,0x0000},
    {"Noite ambar",0x0000,0xffd4,0x1082,0xbff2,0xafff,0xffd4,0x0000},
    {"Claro",0xffff,0x0000,0xe71c,0x0220,0x0010,0x0010,0xffff},
    {"Preto alto contraste",0x0000,0xffff,0x1082,0x07f0,0x07ff,0xffff,0x0000}
};
constexpr unsigned kThemeCount=sizeof(themes)/sizeof(themes[0]);
inline const Theme& theme(unsigned i){return themes[i<kThemeCount?i:0];}
}

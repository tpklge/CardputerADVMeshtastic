#include "cardputer_adv/ui_shell.hpp"
#include "cardputer_adv/mesh_backend.hpp"
#include <cstring>
namespace cardputer_adv {
void ShellMenu::begin(std::uint32_t now,ShellTab tab){tab_=tab;open(now);}
bool ShellMenu::select(ShellTab tab,std::uint32_t now){tick(now);if(tab==tab_)return false;tab_=tab;open(now);return true;}
void ShellMenu::open(std::uint32_t now){opened_=now;visible_=true;}
bool ShellMenu::tick(std::uint32_t now){if(visible_ && std::uint32_t(now-opened_)>=5000){visible_=false;return true;}return false;}
bool ShellMenu::navigate(bool backwards,std::uint32_t now,bool blocked){
    tick(now);
    if(!visible_){open(now);return false;}
    if(blocked)return false;
    return select(ShellTab((unsigned(tab_)+(backwards?3:1))%4),now);
}
std::size_t TextEditor::length() const{return std::strlen(text_);}
std::size_t TextEditor::next(const char* text,std::size_t at){
    if(!text[at])return at;
    auto byte=std::uint8_t(text[at]);unsigned size=byte>=0xc2 && byte<=0xdf?2:byte>=0xe0 && byte<=0xef?3:byte>=0xf0 && byte<=0xf4?4:1;
    for(unsigned i=1;i<size;++i)if(!text[at+i] || (std::uint8_t(text[at+i])&0xc0)!=0x80)return at+1;
    return at+size;
}
bool TextEditor::set(const char* text,std::size_t cursor){
    if(!text || !capacity_ || !mesh_valid_utf8(text,capacity_,true))return false;
    auto size=std::strlen(text);if(cursor>size || (cursor<size && (std::uint8_t(text[cursor])&0xc0)==0x80))return false;
    std::memmove(text_,text,size+1);cursor_=cursor;return true;
}
bool TextEditor::insert(char character){auto size=length();if(character<32 || character>126 || size+1>=capacity_)return false;std::memmove(text_+cursor_+1,text_+cursor_,size-cursor_+1);text_[cursor_++]=character;return true;}
bool TextEditor::left(){if(!cursor_)return false;do{--cursor_;}while(cursor_ && (std::uint8_t(text_[cursor_])&0xc0)==0x80);return true;}
bool TextEditor::right(){if(!text_[cursor_])return false;cursor_=next(text_,cursor_);return true;}
bool TextEditor::backspace(){auto end=cursor_;if(!left())return false;std::memmove(text_+cursor_,text_+end,length()-end+1);return true;}
bool TextEditor::set_limit(std::size_t bytes){if(bytes>=bound_ || length()>bytes)return false;capacity_=bytes+1;return true;}
std::uint16_t shell_local_minute(std::uint32_t utc,std::int16_t offset){auto minute=(std::int64_t(utc/60)+offset)%1440;return minute<0?minute+1440:minute;}
void TextEditor::clear(){text_[0]=0;cursor_=0;}
}

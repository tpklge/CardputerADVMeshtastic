#pragma once
#include "keyboard_decoder.hpp"
#include <cstddef>
#include <cstdint>
namespace cardputer_adv {
enum class ShellTab:std::uint8_t {Chat,Nodes,Channels,Settings};
struct UiNavigation {
    bool shell=false,conversations=false;
    std::uint8_t theme=0;
    std::uint8_t chat_channel=0;
    std::uint32_t chat_peer=0; // zero selects a channel conversation
    std::uint64_t read_sequence=0;
    std::uint32_t node_offset=0;
    bool node_favorites=false;
    char node_query[25]{};
    std::uint64_t history_before=0,detail_sequence=0;
    std::uint16_t detail_row=0;
    std::uint8_t node_selection=0,channel_selection=0,settings_selection=0,history_selection=0,history_first=0;
};
class ShellMenu {
    ShellTab tab_=ShellTab::Chat;
    std::uint32_t opened_=0;
    bool visible_=false;
public:
    void begin(std::uint32_t now,ShellTab tab=ShellTab::Chat);
    bool tick(std::uint32_t now);
    bool navigate(bool backwards,std::uint32_t now,bool blocked=false);
    void open(std::uint32_t now);
    bool select(ShellTab tab,std::uint32_t now);
    void close(){visible_=false;}
    bool visible() const{return visible_;}
    ShellTab tab() const{return tab_;}
};
class TextEditor {
    char* text_;
    std::size_t capacity_,bound_,cursor_=0;
public:
    TextEditor(char* text,std::size_t capacity):text_(text),capacity_(capacity),bound_(capacity){}
    bool set(const char* text,std::size_t cursor=0);
    bool insert(char character);
    bool backspace();
    bool left();
    bool right();
    void clear();
    bool set_limit(std::size_t bytes);
    const char* text() const{return text_;}
    std::size_t cursor() const{return cursor_;}
    std::size_t length() const;
    std::size_t limit() const{return capacity_-1;}
    static std::size_t next(const char* text,std::size_t offset);
};
std::uint16_t shell_local_minute(std::uint32_t utc,std::int16_t offset);
constexpr int kShellWidth=240,kShellHeight=135,kShellTop=16,kShellMenuHeight=18,kShellLine=16;
constexpr int shell_bottom(bool menu){return kShellHeight-(menu?kShellMenuHeight:0);}
constexpr int shell_editor_y(bool menu){return shell_bottom(menu)-kShellLine;}
}

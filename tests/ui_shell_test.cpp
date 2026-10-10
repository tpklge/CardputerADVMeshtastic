#include "cardputer_adv/ui_shell.hpp"
#include <cassert>
#include <cstring>
#include <cstdint>
using namespace cardputer_adv;
int main() {
    ShellMenu menu;menu.begin(100);
    assert(menu.visible() && menu.tab()==ShellTab::Chat);
    assert(!menu.tick(5099) && menu.visible());assert(menu.tick(5100) && !menu.visible());
    assert(!menu.navigate(false,6000) && menu.tab()==ShellTab::Chat && menu.visible());
    assert(menu.navigate(false,6500) && menu.tab()==ShellTab::Nodes);
    assert(!menu.select(ShellTab::Nodes,7000));assert(!menu.tick(11499));assert(menu.tick(11500));
    assert(!menu.navigate(true,12000));assert(menu.navigate(true,12100) && menu.tab()==ShellTab::Chat);
    menu.begin(20000,ShellTab::Settings);
    assert(!menu.navigate(false,24999,true));assert(menu.tick(25000) && menu.tab()==ShellTab::Settings);
    // Ordinary input, RX/ticks and scrolling have no menu action.
    char draft[201]{};TextEditor editor(draft,sizeof(draft));menu.begin(30000);
    for(unsigned i=0;i<10;++i){assert(editor.insert('a'));menu.tick(30000+400*i);}
    assert(menu.tick(35000));assert(!std::strcmp(draft,"aaaaaaaaaa"));
    const std::uint32_t near_wrap=UINT32_MAX-1000u;menu.begin(near_wrap);
    assert(!menu.tick(near_wrap+4999u));assert(menu.tick(near_wrap+5000u));
    assert(editor.set(u8"ação🛰",std::strlen(u8"ação🛰")));
    auto total=editor.length();assert(editor.left() && editor.cursor()==total-4);assert(editor.right() && editor.cursor()==total);
    assert(editor.backspace() && !std::strcmp(draft,u8"ação"));
    assert(!editor.set(u8"ação",2)); // cursor cannot split a UTF-8 codepoint
    assert(editor.set("ab",1));assert(editor.insert('X'));assert(!std::strcmp(draft,"aXb"));assert(editor.backspace());assert(!std::strcmp(draft,"ab"));
    assert(!editor.set_limit(0));editor.clear();assert(editor.set_limit(4));for(unsigned i=0;i<4;++i)assert(editor.insert('x'));assert(!editor.insert('y'));assert(editor.set_limit(200));
    const char truncated[]={'x',char(0xc3),0};assert(TextEditor::next(truncated,1)==2);
    editor.clear();for(unsigned i=0;i<200;++i)assert(editor.insert('x'));assert(!editor.insert('y') && editor.length()==200);
    char url[2048]{};TextEditor link(url,sizeof(url));const char* value="https://meshtastic.org/e/#Ab_C-09+=/?";
    assert(link.set(value,std::strlen(value)));assert(!std::strcmp(link.text(),value));
    link.clear();for(unsigned i=0;i<2047;++i)assert(link.insert('a'));assert(!link.insert('b'));
    assert(shell_local_minute(0,-240)==1200);assert(shell_local_minute(3600,840)==900);
    assert(shell_bottom(true)==117 && shell_bottom(false)==135);
    assert(shell_editor_y(true)+16==shell_bottom(true));assert(shell_editor_y(false)+16==shell_bottom(false));
}

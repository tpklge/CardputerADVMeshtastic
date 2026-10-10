#ifdef CARDPUTER_ADV_M6
#include "cardputer_adv/channels.hpp"
#include "cardputer_adv/ui_shell.hpp"
#include "cardputer_adv/app_storage.hpp"
#include "cardputer_adv/m2_console.hpp"
#include "cardputer_adv/shell_display.hpp"
#include <esp_system.h>
#include <cstring>
#include <cstdio>
namespace cardputer_adv {namespace {
enum class Page{List,Actions,Name,Key,EditReview,Import,Mode,Review,Confirm,Share,Url};Page page=Page::List;
ChannelPlan plan;meshtastic_ChannelSet imported{};char value[2048]{};TextEditor editor(value,sizeof(value));char message[100]{};unsigned slot=0,action=0,review=0;bool replace=false,radio=false,loaded=false;std::uint32_t operation=0,revision=0,next_token=0,expected_export=0;
const char* presets[]={"LONG_FAST","LONG_SLOW","VERY_LONG_SLOW","MEDIUM_SLOW","MEDIUM_FAST","SHORT_SLOW","SHORT_FAST","LONG_MODERATE","SHORT_TURBO","LONG_TURBO","LITE_FAST","LITE_SLOW","NARROW_FAST","NARROW_SLOW"};
const char* actions[]={"Editar / criar","Gerar chave AES256","Desativar secundario","Importar link","Exportar canal","Exportar conjunto"};
void say(const char* s){std::snprintf(message,sizeof(message),"%s",s);}
void commit(){if(operation){say("Aguarde a operacao anterior");return;}operation=++next_token;auto token=0x80000000u|operation;if(!mesh_channels_apply(plan,token)){operation=0;say("Nucleo/SD ocupado; tente de novo");return;}say("Salvando e aplicando canais");page=Page::List;}
void restore_url(){std::uint16_t cursor=0;char draft[2048];app_storage_url_load(draft,sizeof(draft),cursor);editor.clear();editor.set_limit(2047);editor.set(draft,cursor);}
void begin_review(){ChannelPlan current;if(!mesh_channels_snapshot(current) || !channel_plan_import(current,imported,replace,radio)){say("Sem slots / link ou LoRa invalido");return;}plan=current;review=0;page=Page::Review;say(replace?"Substitui canais; Enter confirma":imported.has_lora_config?"Adicionar ignora LoRa do link":"Mantem primario/LoRa atuais");}
}
bool m6_channels_root(){return page==Page::List;}
void m6_channels_event(const MeshEvent& e){if(operation && e.token==(0x80000000u|operation)){operation=0;say(e.kind==MeshEventKind::Configured?"Canais salvos/aplicados":e.text);}}
void m6_channels_tick(){auto storage=app_storage_status();if(storage.fault){operation=0;expected_export=0;say("SD erro: gravacao nao confirmada");}else if(expected_export && storage.exports>=expected_export){expected_export=0;say("Link salvo em exports/*.url");}if(!loaded && app_storage_status().ready){restore_url();loaded=true;}auto now=mesh_channels_revision();if(revision && now!=revision){operation=0;say("Canais atualizados");}revision=now;if(operation && (mesh_snapshot().storage_fault || app_storage_status().fault)){operation=0;say("Falha SD: confira configuracao");}}
void m6_channels_key(const KeyEvent& e,std::uint8_t selected){if(page==Page::List && selected<8)slot=selected;if(operation){say("Aguarde gravacao dos canais");return;}if(expected_export && page!=Page::Url && e.action!=KeyAction::Escape){say("Aguarde exportacao no SD");return;}if(page==Page::List){if(e.action==KeyAction::Enter){if(!mesh_channels_snapshot(plan)){say("Nucleo indisponivel");return;}page=Page::Actions;action=0;}return;}
 if(e.action==KeyAction::Escape){page=Page::List;say("Operacao cancelada");return;}
 if(page==Page::Name || page==Page::Key || page==Page::Import){bool edit=true;switch(e.action){case KeyAction::Text:if(!editor.insert(e.character))say("Limite do campo");break;case KeyAction::Backspace:editor.backspace();break;case KeyAction::Left:editor.left();break;case KeyAction::Right:editor.right();break;default:edit=false;break;}if(page==Page::Import && edit)app_storage_url(editor.text(),editor.cursor());if(edit)return;if(e.action!=KeyAction::Enter || e.repeat)return;
  if(page==Page::Name){if(!mesh_valid_utf8(editor.text(),12) || !editor.text()[std::strspn(editor.text()," ")]){say("Nome: 1 a 11 bytes");return;}auto& c=plan.slots[slot];std::strcpy(c.settings.name,editor.text());char key[65];channel_key_text(c.settings.psk,key);editor.clear();editor.set_limit(64);editor.set(key,std::strlen(key));page=Page::Key;say("Hex/Base64; vazio herda; 00 aberta");}
  else if(page==Page::Key){if(!channel_key_parse(editor.text(),plan.slots[slot].settings.psk)){say("PSK: 0/1/16/32 bytes");return;}page=Page::EditReview;say("Enter aplica; Esc cancela");}
  else {bool add=false;if(!channel_url_decode(editor.text(),&imported,&add)){say("Link Meshtastic invalido");return;}replace=!add;radio=false;page=Page::Mode;say("Setas: modo; dir: LoRa");}return;}
 if(page==Page::Actions){if(e.action==KeyAction::Up && action)--action;else if(e.action==KeyAction::Down && action<5)++action;else if(e.action==KeyAction::Enter && !e.repeat){
  if(action==0){auto& c=plan.slots[slot];if(!c.role){c={};c.index=slot;c.role=meshtastic_Channel_Role_SECONDARY;c.has_settings=true;c.settings.psk.size=32;esp_fill_random(c.settings.psk.bytes,32);}editor.clear();editor.set_limit(11);editor.set(c.settings.name,std::strlen(c.settings.name));page=Page::Name;say("Enter: proximo campo");}
  else if(action==1){if(!plan.slots[slot].role){say("Crie o canal primeiro");return;}plan.slots[slot].settings.psk.size=32;esp_fill_random(plan.slots[slot].settings.psk.bytes,32);page=Page::EditReview;say("Nova chave; Enter confirma");}
  else if(action==2){if(plan.slots[slot].role!=meshtastic_Channel_Role_SECONDARY){say("Somente secundario pode desativar");return;}plan.slots[slot]={};plan.slots[slot].index=slot;page=Page::EditReview;say("Desativar? Enter confirma");}
  else if(action==3){restore_url();page=Page::Import;say("Digite o link; Enter: revisar");}
  else {page=Page::Share;say("Contem chaves; Enter revela/salva");}}
 }else if(page==Page::Mode){if(e.action==KeyAction::Up || e.action==KeyAction::Down){replace=!replace;if(!replace)radio=false;}else if(e.action==KeyAction::Right && replace && imported.has_lora_config)radio=!radio;else if(e.action==KeyAction::Enter)begin_review();}
 else if(page==Page::Review){if(e.action==KeyAction::Up && review)--review;else if(e.action==KeyAction::Down && review<7)++review;else if(e.action==KeyAction::Enter)page=Page::Confirm;}
 else if((page==Page::Confirm || page==Page::EditReview) && e.action==KeyAction::Enter && !e.repeat)commit();
 else if(page==Page::Share && e.action==KeyAction::Enter && !e.repeat){editor.clear();editor.set_limit(2047);if(!mesh_channels_export(action==4?int(slot):-1,value,sizeof(value))){say("Falha na exportacao");return;}editor.set(value,0);if(!app_storage_url(value,0,true)){say("SD indisponivel");return;}expected_export=app_storage_status().exports+1;page=Page::Url;say("Exportacao na fila SD; setas: ler");}
 else if(page==Page::Url){if(e.action==KeyAction::Left || e.action==KeyAction::Up)editor.left();else if(e.action==KeyAction::Right || e.action==KeyAction::Down)editor.right();}
}
void m6_channels_draw(ShellDisplay& d,int bottom,std::uint8_t selected){if(page==Page::List)slot=selected;auto line=[&](int y,const char* s){char text[128]{};std::size_t n=0;while(s[n]){auto end=TextEditor::next(s,n);if(end>=sizeof(text) || d.getStringWidth(s,end,true)>236)break;n=end;}std::memcpy(text,s,n);d.drawString(2,y,text);};char buf[120];line(16,"CANAIS");
 if(page==Page::List){MeshChannel channels[8];auto count=mesh_channels_page(0,channels,8);unsigned first=(slot/4)*4;for(unsigned i=first;i<count && i<first+4;++i){std::snprintf(buf,sizeof(buf),"%c %u %s",i==slot?'>':' ',i,channels[i].role?channels[i].name:"Livre");line(32+16*(i-first),buf);}line(bottom-16,message[0]?message:"Enter: acoes do canal");return;}
 if(page==Page::Actions){unsigned first=(action/4)*4;for(unsigned i=first;i<6 && i<first+4;++i){std::snprintf(buf,sizeof(buf),"%c %s",i==action?'>':' ',actions[i]);line(32+16*(i-first),buf);}}
 else if(page==Page::Name || page==Page::Key || page==Page::Import || page==Page::Url){line(32,page==Page::Name?"Nome do canal":page==Page::Key?"Chave (Hex/Base64)":"Link Meshtastic");std::size_t start=0;while(start<editor.cursor() && d.getStringWidth(value+start,editor.cursor()-start,true)>216)start=TextEditor::next(value,start);line(48,value+start);d.drawHorizontalLine(2+d.getStringWidth(value+start,editor.cursor()-start,true),63,4);std::snprintf(buf,sizeof(buf),"%u/%u bytes",unsigned(editor.length()),unsigned(editor.limit()));line(64,buf);}
 else if(page==Page::Mode){line(32,replace?"Modo: SUBSTITUIR":"Modo: ADICIONAR");line(48,radio?"LoRa do link: APLICAR":"LoRa atual: PRESERVAR");std::snprintf(buf,sizeof(buf),"%u canais no link",unsigned(imported.settings_count));line(64,buf);auto& l=radio?imported.lora_config:plan.lora;std::snprintf(buf,sizeof(buf),"%s / slot %lu",l.use_preset && unsigned(l.modem_preset)<14?presets[unsigned(l.modem_preset)]:"CUSTOM",static_cast<unsigned long>(l.channel_num));line(80,buf);}
 else if(page==Page::Review){auto& c=plan.slots[review];std::snprintf(buf,sizeof(buf),"Slot %u: %s",review,c.role?c.settings.name:"Desativado");line(32,buf);line(48,c.role==1?"Primario":c.role==2?"Secundario":"Livre");std::snprintf(buf,sizeof(buf),"PSK: %u bytes (oculta)",unsigned(c.settings.psk.size));line(64,buf);line(80,plan.apply_lora?"LoRa sera alterado":"LoRa sera preservado");}
 else if(page==Page::EditReview){auto& c=plan.slots[slot];std::snprintf(buf,sizeof(buf),"Slot %u: %s",slot,c.settings.name);line(32,buf);line(48,c.role?"Salvar nome/chave":"Desativar canal");std::snprintf(buf,sizeof(buf),"PSK: %u bytes (oculta)",unsigned(c.settings.psk.size));line(64,buf);line(80,c.role==1?"Primario: pode alterar RF":"Enter confirma; Esc cancela");}
 else if(page==Page::Confirm){line(32,"Confirmar conjunto revisado?");line(48,replace?"Primario pode alterar RF":"Alteracoes serao salvas no SD");line(64,"Enter aplica; Esc cancela");}
 else {line(32,"Exportar link com chaves?");line(48,"Arquivo .url no microSD");line(64,"Enter revela; Esc cancela");}line(bottom-16,message);
}
}
#endif

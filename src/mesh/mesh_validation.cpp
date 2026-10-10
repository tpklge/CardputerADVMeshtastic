#include "cardputer_adv/mesh_backend.hpp"
#include <cstring>
namespace cardputer_adv {
bool mesh_valid_utf8(const char* s,std::size_t cap,bool empty,bool multiline) {
    if(!s || !cap) return false;
    std::size_t length=0; while(length<cap && s[length]) ++length;
    if(length==cap || (!length&&!empty)) return false;
    for(std::size_t i=0;i<length;) {
        auto c=static_cast<unsigned char>(s[i++]); if(c<0x80) { if((c<0x20 && !(multiline && (c=='\n' || c=='\r' || c=='\t'))) || c==0x7f) return false; continue; }
        unsigned n; std::uint32_t value,min;
        if(c>=0xc2&&c<=0xdf) {n=1;value=c&31;min=0x80;}
        else if(c>=0xe0&&c<=0xef) {n=2;value=c&15;min=0x800;}
        else if(c>=0xf0&&c<=0xf4) {n=3;value=c&7;min=0x10000;}
        else return false;
        if(i+n>length) return false;
        while(n--) { auto b=static_cast<unsigned char>(s[i++]); if((b&0xc0)!=0x80) return false; value=(value<<6)|(b&63); }
        if(value<min || value>0x10ffff || (value>=0xd800&&value<=0xdfff)) return false;
    }
    return true;
}
bool mesh_region_supported(std::uint8_t r) {
    // Official Cap antenna covers 868/915 MHz; exclude 433 MHz, 2 m and 2.4 GHz.
    switch(r) {case 0: case 1: case 3: case 5: case 6: case 7: case 8: case 9: case 10: case 11: case 12:
    case 15: case 17: case 18: case 20: case 21: case 24: case 25: case 26: case 29: case 30: case 31: case 32: return true; default:return false;}
}
bool mesh_validate_command(const MeshCommand& c) {
    switch(c.kind) {
    #ifdef CARDPUTER_ADV_M8
    case MeshCommandKind::TraceRoute:return c.destination>1 && c.destination!=0xffffffff;
#endif
    case MeshCommandKind::SendText: return c.channel<8 && c.destination>1 && mesh_valid_utf8(c.text,sizeof(c.text));
#ifdef CARDPUTER_ADV_M7_NODES
    case MeshCommandKind::SetFavorite:return c.destination>1 && c.destination!=0xffffffff;
#endif
    case MeshCommandKind::SetOwner: return mesh_valid_utf8(c.long_name,sizeof(c.long_name)) && mesh_valid_utf8(c.short_name,sizeof(c.short_name)) && c.long_name[std::strspn(c.long_name," ")] && c.short_name[std::strspn(c.short_name," ")];
    case MeshCommandKind::SetRadio: return mesh_region_supported(c.region) && c.preset<=13 && c.hop_limit<=7;
    default:return false;
    }
}
}

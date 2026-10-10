#include "cardputer_adv/mesh_backend.hpp"
#include <cassert>
#include <cstring>
using namespace cardputer_adv;
int main() {
    BoundedQueue<MeshEvent,2> queue;MeshEvent event;event.kind=MeshEventKind::Text;std::strcpy(event.text,"original");
    assert(queue.push(event));std::strcpy(event.text,"changed");assert(queue.push(event));assert(!queue.push(event));
    MeshEvent out;assert(queue.pop(out));assert(std::strcmp(out.text,"original")==0);assert(queue.push(event));
    assert(queue.pop(out));assert(queue.pop(out));assert(!queue.pop(out));
    CommandResults results;
    BoundedQueue<MeshEvent,16> flooded;
    for(unsigned i=0;i<16;++i)assert(flooded.push(event));assert(!flooded.push(event));
    for(unsigned i=1;i<=8;++i){assert(results.reserve());event.token=i;assert(results.push(event));}
    assert(!results.reserve());
    for(unsigned i=1;i<=8;++i){assert(results.pop(out) && out.token==i);}
    assert(!results.pop(out));assert(results.reserve());results.cancel();
    assert(mesh_valid_utf8("linha 1\nlinha 2\tOK\r",32,false,true));
    assert(!mesh_valid_utf8("controle\x01",16,false,true));
    MeshCommand c;std::strcpy(c.text,"Oi, a\xc3\xa7\xc3\xa3o");assert(mesh_validate_command(c));
    c.text[0]=0;assert(!mesh_validate_command(c));std::memset(c.text,'a',sizeof(c.text));assert(!mesh_validate_command(c));
    c.text[kMeshTextBytes]=0;assert(mesh_validate_command(c));c.channel=8;assert(!mesh_validate_command(c));c.channel=0;
    c.destination=0;assert(!mesh_validate_command(c));c.destination=0xffffffff;
    assert(!mesh_valid_utf8("\xc0\xaf",3));assert(!mesh_valid_utf8("\xed\xa0\x80",4));assert(!mesh_valid_utf8("\xf4\x90\x80\x80",5));
    assert(!mesh_valid_utf8("\xe2\x82",3));assert(!mesh_valid_utf8("abc\n",5));assert(mesh_valid_utf8("\xf0\x9f\x93\xa1",5));
    c.kind=MeshCommandKind::SetOwner;std::strcpy(c.long_name,"Cardputer ADV");std::strcpy(c.short_name,"ADV");assert(mesh_validate_command(c));
    std::memset(c.short_name,'a',sizeof(c.short_name));assert(!mesh_validate_command(c));
    c.kind=MeshCommandKind::SetRadio;c.region=0;assert(mesh_validate_command(c));c.region=26;assert(mesh_validate_command(c));
    for(auto r:{2,13,27,33,255}){c.region=r;assert(!mesh_validate_command(c));}
    c.region=26;c.preset=14;assert(!mesh_validate_command(c));c.preset=0;c.hop_limit=8;assert(!mesh_validate_command(c));
    c.hop_limit=3;c.frequency_slot=0;assert(mesh_validate_command(c));
    c.kind=MeshCommandKind::SetOwner;std::strcpy(c.short_name,"    ");assert(!mesh_validate_command(c));
}

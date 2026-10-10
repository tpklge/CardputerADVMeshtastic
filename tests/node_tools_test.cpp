#include "cardputer_adv/node_tools.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
using namespace cardputer_adv;
namespace cardputer_adv {
MeshSnapshot mesh_snapshot(){MeshSnapshot s;s.nodes=9;return s;}
std::size_t mesh_nodes_page(std::size_t offset,MeshNode* out,std::size_t capacity){auto n=offset<9?std::min(capacity,9-offset):0;for(unsigned i=0;i<n;++i){out[i]={};out[i].number=0x1234+offset+i;out[i].has_user=true;out[i].favorite=(offset+i)%2==0;std::strcpy(out[i].long_name,"Alpha");}return n;}
}
int main(){MeshNode n;n.has_user=true;n.number=0xabcdef01;std::strcpy(n.long_name,"Estacao Alpha");std::strcpy(n.short_name,"A");assert(node_matches(n,"aLPha") && node_matches(n,"!ABCDEF01") && node_matches(n,"def0"));assert(!node_matches(n,"beta") && !node_matches(n,"alpha",true));n.favorite=true;assert(node_matches(n,"alpha",true));n.has_user=false;assert(!node_matches(n,"Alpha"));
MeshNode page[4];std::size_t total;assert(mesh_nodes_find(0,"alpha",true,page,4,total)==4 && total==5 && page[3].number==0x123a);assert(mesh_nodes_find(4,"alpha",true,page,4,total)==1 && page[0].number==0x123c);assert(mesh_nodes_find(0,"unknown",false,page,4,total)==0 && total==0);
MeshSnapshot local;local.gps_fix=true;local.gps_time=true;local.utc=1800000000;local.gps_fix_millis=1000;n.has_position=true;n.position_time=local.utc;n.latitude=10000000;n.longitude=0;auto d=node_distance(local,n,1000);assert(d.valid && d.has_bearing && std::abs(d.metres-111194.9266)<1 && std::abs(d.bearing)<1e-7);n.latitude=0;n.longitude=10000000;d=node_distance(local,n,1000);assert(d.valid && std::abs(d.bearing-90)<1e-7);n.longitude=-10000000;d=node_distance(local,n,1000);assert(std::abs(d.bearing-270)<1e-7);
n.longitude=0;d=node_distance(local,n,1000);assert(d.valid && d.metres==0 && !d.has_bearing);n.position_time=local.utc-901;assert(!node_distance(local,n,1000).valid);n.position_time=local.utc+1;assert(!node_distance(local,n,1000).valid);n.position_time=local.utc;assert(!node_distance(local,n,121001).valid);local.gps_fix=false;assert(!node_distance(local,n,1000).valid);local.gps_fix=true;n.latitude=900000001;assert(!node_distance(local,n,1000).valid);
local.longitude=1799000000;n.latitude=0;n.longitude=-1799000000;d=node_distance(local,n,1000);assert(d.valid && d.metres<23000 && d.metres>22000);local.gps_time=false;assert(!node_distance(local,n,1000).valid);
}

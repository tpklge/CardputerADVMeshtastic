#include "cardputer_adv/node_tools.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
namespace cardputer_adv {
namespace {
unsigned char lower(unsigned char c){return c>='A' && c<='Z'?c+32:c;}
bool contains(const char* haystack,const char* needle){if(!*needle)return true;for(;*haystack;++haystack){auto a=haystack,b=needle;while(*a && *b && lower(*a)==lower(*b)){++a;++b;}if(!*b)return true;}return false;}
bool coordinates(std::int32_t lat,std::int32_t lon){return lat>=-900000000 && lat<=900000000 && lon>=-1800000000 && lon<=1800000000;}
}
bool node_matches(const MeshNode& node,const char* query,bool favorites_only){if(!query || (favorites_only && !node.favorite))return false;char id[10];std::snprintf(id,sizeof(id),"!%08lx",static_cast<unsigned long>(node.number));return contains(id,query) || (node.has_user && (contains(node.short_name,query) || contains(node.long_name,query)));}
std::size_t mesh_nodes_find(std::size_t offset,const char* query,bool favorites_only,MeshNode* out,std::size_t capacity,std::size_t& total){total=0;std::size_t written=0;capacity=std::min<std::size_t>(capacity,4);if(!out || !query)return 0;const auto limit=mesh_snapshot().nodes;for(std::size_t at=0;at<limit;at+=4){MeshNode nodes[4];auto count=mesh_nodes_page(at,nodes,4);if(!count)break;for(unsigned i=0;i<count;++i)if(node_matches(nodes[i],query,favorites_only)){if(total>=offset && written<capacity)out[written++]=nodes[i];++total;}}return written;}
NodeDistance node_distance(const MeshSnapshot& local,const MeshNode& remote,std::uint32_t now){NodeDistance out;if(!local.gps_fix || !local.gps_time || std::uint32_t(now-local.gps_fix_millis)>120000 || !remote.has_position || !remote.position_time || remote.position_time>local.utc || local.utc-remote.position_time>900 || !coordinates(local.latitude,local.longitude) || !coordinates(remote.latitude,remote.longitude))return out;constexpr double pi=3.14159265358979323846,rad=pi/180;auto lat1=local.latitude/1e7*rad,lat2=remote.latitude/1e7*rad;auto dlat=lat2-lat1,dlon=(remote.longitude/1e7-local.longitude/1e7)*rad;auto a=std::pow(std::sin(dlat/2),2)+std::cos(lat1)*std::cos(lat2)*std::pow(std::sin(dlon/2),2);a=std::max(0.,std::min(1.,a));out.metres=6371000*2*std::atan2(std::sqrt(a),std::sqrt(1-a));out.valid=std::isfinite(out.metres);auto y=std::sin(dlon)*std::cos(lat2),x=std::cos(lat1)*std::sin(lat2)-std::sin(lat1)*std::cos(lat2)*std::cos(dlon);out.has_bearing=out.valid && std::hypot(x,y)>1e-12;out.bearing=out.has_bearing?std::fmod(std::atan2(y,x)/rad+360,360):0;return out;}
}

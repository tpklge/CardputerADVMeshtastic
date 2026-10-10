#pragma once
#include "mesh_backend.hpp"
namespace cardputer_adv {
bool node_matches(const MeshNode& node,const char* query,bool favorites_only=false);
std::size_t mesh_nodes_find(std::size_t offset,const char* query,bool favorites_only,MeshNode* out,std::size_t capacity,std::size_t& total);
struct NodeDistance {double metres=0,bearing=0;bool valid=false,has_bearing=false;};
NodeDistance node_distance(const MeshSnapshot& local,const MeshNode& remote,std::uint32_t now);
}

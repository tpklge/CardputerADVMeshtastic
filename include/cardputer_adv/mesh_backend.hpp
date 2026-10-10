#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace cardputer_adv {
constexpr std::size_t kMeshTextBytes = 200;
template<class T, std::size_t N> class BoundedQueue {
    std::array<T,N> entries_{};
    std::size_t head_=0, count_=0;
public:
    bool push(const T& value) { if(count_==N) return false; entries_[(head_+count_)%N]=value; ++count_; return true; }
    bool pop(T& value) { if(!count_) return false; value=entries_[head_]; head_=(head_+1)%N; --count_; return true; }
    std::size_t size() const { return count_; }
};
#ifdef CARDPUTER_ADV_M8
struct TraceSnapshot {bool available=false,tracking=false,truncated=false;std::uint32_t node=0,cooldown_ms=0;char text[1024]{};};
TraceSnapshot mesh_trace_snapshot();
void mesh_observe_loop(std::uint32_t duration_ms);
#endif
enum class MeshCommandKind : std::uint8_t { SendText, SetOwner, SetRadio
#ifdef CARDPUTER_ADV_M8
    ,TraceRoute
#endif
#ifdef CARDPUTER_ADV_M7_NODES
    ,SetFavorite
#endif
};
struct MeshCommand {
    MeshCommandKind kind=MeshCommandKind::SendText;
    std::uint32_t token=0, destination=0xffffffff;
    std::uint64_t expected_conversation=0;
    std::uint16_t frequency_slot=0;
    std::uint8_t channel=0, region=0, preset=0, hop_limit=3;
    bool favorite=false;
    char text[kMeshTextBytes+1]{};
    char long_name[40]{}, short_name[5]{};
};
enum class MeshEventKind : std::uint8_t { Text, Position, Telemetry, Node, Queued, Transmitted, RoutingFeedback, Configured, Error };
struct MeshEvent {
    MeshEventKind kind=MeshEventKind::Error;
    std::uint32_t token=0, packet_id=0, node=0, destination=0, time=0;
    std::int32_t latitude=0, longitude=0, altitude=0;
    std::int32_t result=0;
    float rssi=0, snr=0, voltage=0;
    std::uint8_t channel=0, battery=0,hop_start=0,hop_limit=0;
    bool has_position=false,has_voltage=false,has_battery=false;
    char text[kMeshTextBytes+1]{};
};
// Caller supplies the queue lock. Reservations include queued commands and
// completed results, so accepting more work can never discard a confirmation.
class CommandResults {
    BoundedQueue<MeshEvent,8> queue_;
    unsigned reserved_=0;
public:
    bool reserve(){if(reserved_==8)return false;++reserved_;return true;}
    void cancel(){if(reserved_)--reserved_;}
    bool push(const MeshEvent& event){return queue_.push(event);}
    bool pop(MeshEvent& event){if(!queue_.pop(event))return false;--reserved_;return true;}
};
struct MeshNode {
    std::uint32_t number=0,last_heard=0;
    std::int32_t latitude=0,longitude=0;
    float snr=0,voltage=0;
    std::uint32_t heard_millis=0,position_time=0;
    std::int32_t altitude=0;
    std::uint8_t hops=0,battery=0;
    bool has_hops=false,has_voltage=false,has_battery=false;
    bool favorite=false;
    bool has_user=false,has_position=false,heard_this_boot=false,heard_known=true;
    char short_name[5]{},long_name[40]{};
};
struct MeshChannel {std::uint8_t index=0,role=0;bool encrypted=false;char name[24]{};};
std::size_t mesh_channels_page(std::size_t offset,MeshChannel* channels,std::size_t capacity);
struct MeshSnapshot {
    bool ready=false, radio_ready=false, storage_ready=false, storage_fault=false;
    bool gps_fix=false, gps_time=false;
    std::uint32_t node=0,tx=0,rx=0,rx_errors=0,event_drops=0,command_drops=0;
    std::uint32_t nodes=0,utc=0,heap_min=0,gps_fix_millis=0;
    std::uint32_t heap_free=0,loop_max_ms=0,uptime_ms=0;
    std::int32_t latitude=0,longitude=0,altitude=0;
    std::uint8_t satellites=0,region=0,preset=0,hop_limit=3;
    std::uint16_t battery_mv=0,frequency_slot=0;
    std::uint8_t battery_percent=0;bool battery_percent_known=false;
    char channel_name[24]{},long_name[40]{},short_name[5]{};
};
bool mesh_validate_command(const MeshCommand& command);
bool mesh_valid_utf8(const char* text, std::size_t capacity, bool allow_empty=false, bool multiline=false);
bool mesh_region_supported(std::uint8_t region);
void mesh_backend_begin();
void mesh_backend_tick();
bool mesh_submit(const MeshCommand& command);
bool mesh_poll_event(MeshEvent& event);
MeshSnapshot mesh_snapshot();
bool mesh_node_name(std::uint32_t number,char* name,std::size_t capacity);
std::size_t mesh_nodes_page(std::size_t offset, MeshNode* nodes, std::size_t capacity);
void mesh_radio_transmitted(std::uint32_t node, std::uint32_t packet_id);
}

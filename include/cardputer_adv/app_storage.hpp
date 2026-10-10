#pragma once
#include "store_codec.hpp"
namespace cardputer_adv {
#ifdef CARDPUTER_ADV_M7_NODES
struct NodeMetricsStamp {std::uint32_t node=0,time=0;std::uint64_t sequence=0;float voltage=0;std::uint8_t battery=0;bool has_voltage=false,has_battery=false;};
NodeMetricsStamp app_storage_node_metrics(std::uint32_t node);
#endif
struct StorageStatus {bool ready=false,fault=false,recovered=false,busy=false,switching=false;std::uint32_t dropped=0,corrupt=0,exports=0;std::uint64_t records=0,retired=0;};
bool app_storage_begin();
void app_storage_tick();
#ifdef CARDPUTER_ADV_M8
bool app_storage_flush(std::uint32_t timeout_ms);
#endif
bool app_storage_event(const MeshEvent& event,std::uint64_t conversation,bool synchronous=false);
void app_storage_ui(const char* draft,std::uint16_t cursor,std::uint8_t view,std::uint64_t conversation,bool force=false,const UiNavigation* navigation=nullptr);
bool app_storage_load_ui(StoredRecord& record);
StorageStatus app_storage_status();
bool app_storage_offset(std::int16_t minutes);
void app_storage_page(std::uint64_t before=0,std::uint64_t conversation=0);
void app_storage_page_newer(std::uint64_t after,std::uint64_t conversation);
bool app_storage_page_latest();
#ifdef CARDPUTER_ADV_M5
struct ConversationStatus {std::uint64_t identity=0,latest=0,read=0;std::uint32_t unread=0;bool scanning=false;};
bool app_storage_bind_channels(const std::uint64_t* identities,std::size_t count);
ConversationStatus app_storage_conversation(std::uint8_t channel);
bool app_storage_select(std::uint64_t conversation,std::uint8_t channel);
#ifdef CARDPUTER_ADV_M7
bool app_storage_select_peer(std::uint64_t conversation,std::uint8_t channel,std::uint32_t peer);
#endif
void app_storage_mark_read(std::uint64_t conversation,std::uint64_t sequence);
#endif
std::size_t app_storage_page_read(StoredRecord* records,std::size_t capacity);
std::uint64_t mesh_conversation(std::uint8_t channel,std::uint32_t peer=0);
}
#ifdef CARDPUTER_ADV_M6
namespace cardputer_adv {
bool app_storage_url(const char* text,std::uint16_t cursor,bool export_file=false);
bool app_storage_url_load(char* text,std::size_t capacity,std::uint16_t& cursor);
}
#endif

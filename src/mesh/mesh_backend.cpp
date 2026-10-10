#ifdef CARDPUTER_ADV_M2
#include "cardputer_adv/mesh_backend.hpp"
#include "cardputer_adv/core_storage.hpp"
#include "MeshService.h"
#include "RadioLibInterface.h"
#include "Router.h"
#include "GPSStatus.h"
#include "PowerStatus.h"
#include "gps/RTC.h"
#include "main.h"
#include <pb_decode.h>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cmath>
#ifdef CARDPUTER_ADV_M3
#include "cardputer_adv/app_storage.hpp"
#include <mbedtls/sha256.h>
#endif
#include <esp_heap_caps.h>
#ifdef CARDPUTER_ADV_M8
#include "modules/TraceRouteModule.h"
#endif
#ifdef CARDPUTER_ADV_M6
#include "cardputer_adv/channels.hpp"
#include <esp_system.h>
#endif

namespace cardputer_adv {
namespace {
BoundedQueue<MeshCommand,6> commands;
BoundedQueue<MeshEvent,16> events;
CommandResults results;
portMUX_TYPE queue_mux=portMUX_INITIALIZER_UNLOCKED;
bool started=false;
#ifdef CARDPUTER_ADV_M8
TraceSnapshot trace;std::uint32_t loop_max_ms=0;
#endif
#ifdef CARDPUTER_ADV_M6
ChannelPlan channel_job;bool channel_pending=false;std::uint32_t channel_token=0,channel_revision=1;
#endif
std::uint32_t event_drops=0,command_drops=0;
std::array<std::uint32_t,200> heard{};
std::array<std::uint32_t,200> heard_at{};
std::size_t heard_count=0;
bool heard_overflow=false;
#ifdef CARDPUTER_ADV_M3
bool nodes_dirty=false;
std::uint32_t nodes_dirty_at=0;
#endif
void publish(const MeshEvent& event,bool already_stored=false) {
#ifdef CARDPUTER_ADV_M3
    if(!already_stored && event.kind!=MeshEventKind::Configured) {
        auto peer=event.destination && event.destination!=NODENUM_BROADCAST ? (event.kind==MeshEventKind::Text?event.node:event.destination):0;
        auto conversation=(event.kind==MeshEventKind::Text || event.kind==MeshEventKind::Queued)?mesh_conversation(event.channel,peer):0;
        app_storage_event(event,conversation);
    }
#endif
    portENTER_CRITICAL(&queue_mux);
    if(event.token && (event.kind==MeshEventKind::Queued || event.kind==MeshEventKind::Configured || event.kind==MeshEventKind::Error)) {
        // Every accepted command owns one reserved result slot until consumed.
        results.push(event);
    } else if(!events.push(event)) ++event_drops;
    portEXIT_CRITICAL(&queue_mux);
}
void error(const MeshCommand& c,const char* message) {
    MeshEvent e; e.kind=MeshEventKind::Error; e.token=c.token;
    std::snprintf(e.text,sizeof(e.text),"%s",message); publish(e);
}
void received(const meshtastic_MeshPacket& p) {
    if((p.from==nodeDB->getNodeNum() || p.from==0) &&
       !(p.which_payload_variant==meshtastic_MeshPacket_decoded_tag && p.decoded.portnum==meshtastic_PortNum_ROUTING_APP)) return;
    if(p.from!=nodeDB->getNodeNum() && p.from && std::find(heard.begin(),heard.begin()+heard_count,p.from)==heard.begin()+heard_count) {
        if(heard_count<heard.size()) heard[heard_count++]=p.from; else heard_overflow=true;
    }
    auto heard_index=std::find(heard.begin(),heard.begin()+heard_count,p.from);
    if(heard_index!=heard.begin()+heard_count)heard_at[heard_index-heard.begin()]=millis();
    if(p.which_payload_variant!=meshtastic_MeshPacket_decoded_tag) return;
    MeshEvent e; e.node=p.from; e.destination=p.to; e.packet_id=p.id; e.time=p.rx_time;
    e.channel=p.channel; e.rssi=p.rx_rssi; e.snr=p.rx_snr;e.hop_start=p.hop_start;e.hop_limit=p.hop_limit;
    auto stream=pb_istream_from_buffer(p.decoded.payload.bytes,p.decoded.payload.size);
    switch(p.decoded.portnum) {
    case meshtastic_PortNum_TEXT_MESSAGE_APP: {
        e.kind=MeshEventKind::Text;
#ifdef CARDPUTER_ADV_M7
        if(p.to!=NODENUM_BROADCAST && (p.to!=nodeDB->getNodeNum() || !p.pki_encrypted || !mesh_conversation(p.channel,p.from)))return;
#endif
        if(p.decoded.payload.size>kMeshTextBytes) {e.kind=MeshEventKind::Error; std::snprintf(e.text,sizeof(e.text),"Texto RX > %u bytes",unsigned(kMeshTextBytes));}
        else {std::memcpy(e.text,p.decoded.payload.bytes,p.decoded.payload.size); e.text[p.decoded.payload.size]=0;
            if(std::strlen(e.text)!=p.decoded.payload.size || !mesh_valid_utf8(e.text,sizeof(e.text),false,true)) {e.kind=MeshEventKind::Error;std::snprintf(e.text,sizeof(e.text),"Texto RX invalido");}}
        break;
    }
    case meshtastic_PortNum_POSITION_APP: {
        meshtastic_Position pos=meshtastic_Position_init_zero;
        if(!pb_decode(&stream,meshtastic_Position_fields,&pos)) return;
        e.kind=MeshEventKind::Position; e.has_position=pos.has_latitude_i&&pos.has_longitude_i; e.latitude=pos.latitude_i; e.longitude=pos.longitude_i; e.altitude=pos.altitude; break;
    }
    case meshtastic_PortNum_TELEMETRY_APP: {
        meshtastic_Telemetry t=meshtastic_Telemetry_init_zero;
        if(!pb_decode(&stream,meshtastic_Telemetry_fields,&t) || t.which_variant!=meshtastic_Telemetry_device_metrics_tag) return;
        e.kind=MeshEventKind::Telemetry; e.has_voltage=t.variant.device_metrics.has_voltage; e.has_battery=t.variant.device_metrics.has_battery_level; e.voltage=t.variant.device_metrics.voltage; e.battery=std::min<std::uint32_t>(t.variant.device_metrics.battery_level,255); break;
    }
    case meshtastic_PortNum_NODEINFO_APP: e.kind=MeshEventKind::Node; break;
    case meshtastic_PortNum_ROUTING_APP: {
        meshtastic_Routing r=meshtastic_Routing_init_zero;
        if(!pb_decode(&stream,meshtastic_Routing_fields,&r) || !p.decoded.request_id || r.which_variant!=meshtastic_Routing_error_reason_tag) return;
        e.kind=MeshEventKind::RoutingFeedback; e.packet_id=p.decoded.request_id; e.result=r.error_reason; break;
    }
    default:return;
    }
#ifdef CARDPUTER_ADV_M3
    if(!nodes_dirty)nodes_dirty_at=millis();nodes_dirty=true;
#endif
    publish(e);
}
void execute(const MeshCommand& c) {
    if(!started || core_storage_fault()) {error(c,"Nucleo/SD indisponivel");return;}
    MeshEvent e; e.token=c.token;
    #ifdef CARDPUTER_ADV_M8
    if(c.kind==MeshCommandKind::TraceRoute){
        if(!traceRouteModule || !RadioLibInterface::instance || error_code || !config.lora.region || !config.lora.tx_enabled){error(c,"Traceroute: radio/regiao indisponivel");return;}
        if(c.destination==nodeDB->getNodeNum() || !nodeDB->getMeshNode(c.destination)){error(c,"Traceroute: escolha outro no conhecido");return;}
        if(traceRouteModule->advState()==TRACEROUTE_STATE_TRACKING || traceRouteModule->advCooldown(millis())){error(c,"Traceroute ocupado: intervalo 30s");return;}
        if(!traceRouteModule->startTraceRoute(c.destination)){error(c,"Traceroute: falha ao iniciar");return;}
        trace={};trace.available=true;trace.tracking=true;trace.node=c.destination;
        std::strcpy(trace.text,"Consultando rota; timeout 10s");
        e.kind=MeshEventKind::Configured;std::strcpy(e.text,"Traceroute iniciado");publish(e);return;
    }
#endif
    if(c.kind==MeshCommandKind::SendText) {
        if(!RadioLibInterface::instance || error_code) {error(c,"Radio indisponivel");return;}
        if(config.lora.region==meshtastic_Config_LoRaConfig_RegionCode_UNSET || !config.lora.tx_enabled) {error(c,"Escolha regiao antes de TX");return;}
        if(channels.getByIndex(c.channel).role==meshtastic_Channel_Role_DISABLED) {error(c,"Canal desativado");return;}
#ifdef CARDPUTER_ADV_M5
        if(c.expected_conversation && c.expected_conversation!=mesh_conversation(c.channel,c.destination==NODENUM_BROADCAST?0:c.destination)){error(c,"Canal mudou: envio cancelado");return;}
#endif
        #ifdef CARDPUTER_ADV_M7
        if(c.destination!=NODENUM_BROADCAST && !mesh_conversation(c.channel,c.destination)){error(c,"DM requer chaves PKI dos dois nos");return;}
#endif
        auto p=router->allocForSending(); if(!p) {error(c,"Sem memoria para pacote");return;}
        p->to=c.destination; p->channel=c.channel; p->want_ack=c.destination!=NODENUM_BROADCAST;
        #ifdef CARDPUTER_ADV_M7
        if(c.destination!=NODENUM_BROADCAST){p->pki_encrypted=true;p->public_key.size=32;std::memcpy(p->public_key.bytes,nodeDB->getMeshNode(c.destination)->user.public_key.bytes,32);}
#endif
        p->decoded.portnum=meshtastic_PortNum_TEXT_MESSAGE_APP;
        p->decoded.payload.size=std::strlen(c.text); std::memcpy(p->decoded.payload.bytes,c.text,p->decoded.payload.size);
        e.kind=MeshEventKind::Queued; e.packet_id=p->id; e.destination=p->to; e.channel=p->channel;
        std::strcpy(e.text,c.text);e.node=nodeDB->getNodeNum();
#ifdef CARDPUTER_ADV_M3
        auto conversation=mesh_conversation(c.channel,c.destination==NODENUM_BROADCAST?0:c.destination);
        if(!app_storage_event(e,conversation,true)){service->releaseToPool(p);error(c,"Historico SD indisponivel: TX cancelado");return;}
#endif
        publish(e,true);
        service->sendToMesh(p,RX_SRC_USER); return;
    }
#ifdef CARDPUTER_ADV_M7_NODES
    if(c.kind==MeshCommandKind::SetFavorite){
        auto node=nodeDB->getMeshNode(c.destination);if(!node){error(c,"No indisponivel");return;}
        auto previous=node->is_favorite;node->is_favorite=c.favorite;
        if(!nodeDB->saveToDisk(SEGMENT_NODEDATABASE)){node->is_favorite=previous;core_storage_set_fault();error(c,"Falha SD ao salvar favorito");return;}
        e.kind=MeshEventKind::Configured;e.node=c.destination;e.result=c.favorite;std::strcpy(e.text,"Favorito salvo no SD");publish(e);return;
    }
#endif
    if(c.kind==MeshCommandKind::SetOwner) {
        auto previous=owner;
        std::memcpy(owner.long_name,c.long_name,sizeof(c.long_name)); std::memcpy(owner.short_name,c.short_name,sizeof(c.short_name));
        if(!nodeDB->saveToDisk(SEGMENT_DEVICESTATE)) {owner=previous; core_storage_set_fault();error(c,"Falha ao salvar nome no SD");return;}
        service->reloadOwner(false);
    } else {
        auto previous=config;
        auto next=config.lora; next.region=static_cast<meshtastic_Config_LoRaConfig_RegionCode>(c.region);
        next.use_preset=true; next.modem_preset=static_cast<meshtastic_Config_LoRaConfig_ModemPreset>(c.preset);
        next.channel_num=c.frequency_slot; next.hop_limit=c.hop_limit; next.override_frequency=0;
        next.tx_enabled=c.region!=0; next.tx_power=0;
        RadioInterface::bootstrapLoRaConfigFromPreset(next);
        const RegionInfo* region=regions;
        while(region->code!=meshtastic_Config_LoRaConfig_RegionCode_UNSET && region->code!=next.region) ++region;
        auto slots=static_cast<unsigned>(std::floor((region->freqEnd-region->freqStart)/(region->spacing+bwCodeToKHz(next.bandwidth)/1000.f)));
        if(next.channel_num && (next.region==meshtastic_Config_LoRaConfig_RegionCode_UNSET || next.channel_num>slots)) {error(c,"Slot fora da faixa da regiao/preset");return;}
        config.lora=next;
        if(c.region && !owner.is_licensed && config.security.public_key.size!=32) {
            if(config.security.private_key.size==32) {
                if(!crypto->regeneratePublicKey(config.security.public_key.bytes,config.security.private_key.bytes)) {config=previous;error(c,"Chave PKI invalida");return;}
            } else crypto->generateKeyPair(config.security.public_key.bytes,config.security.private_key.bytes);
            config.security.private_key.size=32; config.security.public_key.size=32;
        }
        if(!nodeDB->saveToDisk(SEGMENT_CONFIG)) {config=previous;core_storage_set_fault();error(c,"Falha ao salvar radio no SD");return;}
        if(config.security.private_key.size==32) {
            crypto->setDHPrivateKey(config.security.private_key.bytes);
            owner.public_key.size=config.security.public_key.size;
            std::memcpy(owner.public_key.bytes,config.security.public_key.bytes,owner.public_key.size);
        }
        service->reloadOwner(false);
        service->reloadConfig(0);
        if(error_code) {error(c,"Config salva; radio com erro");return;}
    }
    if(core_storage_fault()){error(c,"Falha SD ao aplicar configuracao");return;}
#ifdef CARDPUTER_ADV_M6
    ++channel_revision;
#endif
    e.kind=MeshEventKind::Configured; publish(e);
}
}
#ifdef CARDPUTER_ADV_M3
std::uint64_t mesh_conversation(std::uint8_t channel,std::uint32_t peer) {
    #ifdef CARDPUTER_ADV_M7
    if(peer) {
        auto remote=nodeDB->getMeshNode(peer);
        if(peer==nodeDB->getNodeNum() || !remote || remote->user.public_key.size!=32 || config.security.public_key.size!=32 || config.security.private_key.size!=32 || owner.is_licensed)return 0;
        std::uint8_t context[80]{};context[0]=7;context[1]=1;
        for(unsigned i=0;i<4;++i){context[4+i]=peer>>(i*8);context[8+i]=nodeDB->getNodeNum()>>(i*8);}
        std::memcpy(context+12,remote->user.public_key.bytes,32);std::memcpy(context+44,config.security.public_key.bytes,32);
        std::uint8_t digest[32];if(mbedtls_sha256_ret(context,sizeof(context),digest,0))return 0;
        std::uint64_t id=0;for(unsigned i=0;i<8;++i)id|=std::uint64_t(digest[i])<<(i*8);return id;
    }
#endif
    if(channel>=channels.getNumChannels())return 0;
    auto key=channels.storageKey(channel);
    if(key.length<0)return 0;
    std::uint8_t normalized[96]{};
    auto name=channels.getName(channel);auto length=std::min<std::size_t>(std::strlen(name),40);
    normalized[0]=peer?1:0;normalized[1]=length;std::memcpy(normalized+2,name,length);
    normalized[42]=key.length;std::memcpy(normalized+43,key.bytes,key.length);
    for(unsigned i=0;i<4;++i)normalized[75+i]=peer>>(8*i);
    std::uint8_t digest[32];if(mbedtls_sha256_ret(normalized,sizeof(normalized),digest,0))return 0;
    std::uint64_t identity=0;for(unsigned i=0;i<8;++i)identity|=std::uint64_t(digest[i])<<(i*8);
    std::memset(key.bytes,0,sizeof(key.bytes));std::memset(normalized,0,sizeof(normalized));return identity;
}
#endif
void mesh_backend_begin() {started=service&&router&&nodeDB&&!core_storage_fault();}
bool mesh_submit(const MeshCommand& c) {
#ifdef CARDPUTER_ADV_M6
    if(channel_pending)return false;
#endif
    if(!started || core_storage_fault() || !c.token || !mesh_validate_command(c)) return false;
    portENTER_CRITICAL(&queue_mux); bool reserved=results.reserve();bool ok=reserved && commands.push(c);if(reserved && !ok)results.cancel(); if(!ok) ++command_drops; portEXIT_CRITICAL(&queue_mux);return ok;
}
bool mesh_poll_event(MeshEvent& e) {portENTER_CRITICAL(&queue_mux);bool ok=results.pop(e);if(!ok)ok=events.pop(e);portEXIT_CRITICAL(&queue_mux);return ok;}
void mesh_radio_transmitted(std::uint32_t node,std::uint32_t id) {
    MeshEvent e;e.kind=MeshEventKind::Transmitted;e.node=node;e.packet_id=id;publish(e);
}
#ifdef CARDPUTER_ADV_M8
void mesh_observe_loop(std::uint32_t duration_ms){loop_max_ms=std::max(loop_max_ms,duration_ms);}
TraceSnapshot mesh_trace_snapshot(){auto result=trace;if(traceRouteModule)result.cooldown_ms=traceRouteModule->advCooldown(millis());return result;}
#endif
void mesh_backend_tick() {
    if(!started) return;
#ifdef CARDPUTER_ADV_M8
    if(traceRouteModule && trace.available){
        trace.tracking=traceRouteModule->advState()==TRACEROUTE_STATE_TRACKING;
        if(traceRouteModule->advState()==TRACEROUTE_STATE_RESULT){auto& text=traceRouteModule->advResult();trace.truncated=text.length()>=sizeof(trace.text);auto size=std::min<std::size_t>(text.length(),sizeof(trace.text)-1);if(size<text.length())while(size && (std::uint8_t(text[size])&0xc0)==0x80)--size;std::memcpy(trace.text,text.c_str(),size);trace.text[size]=0;
            if(!std::strcmp(trace.text,"No response received"))std::strcpy(trace.text,"Timeout: sem resposta do destino");}
    }
#endif
#ifdef CARDPUTER_ADV_M6
    if(channel_pending){auto oldFile=channelFile;auto oldConfig=config;auto plan=channel_job;MeshEvent result;result.token=channel_token;result.kind=MeshEventKind::Error;
        bool idle=RadioLibInterface::instance && RadioLibInterface::instance->canSleep() && !RadioLibInterface::instance->isSending();
        bool ok=idle && plan.revision==channel_revision && !core_storage_fault() && channel_plan_valid(plan);
        if(ok && plan.apply_lora){RadioInterface::bootstrapLoRaConfigFromPreset(plan.lora);const RegionInfo* region=regions;while(region->code!=meshtastic_Config_LoRaConfig_RegionCode_UNSET && region->code!=plan.lora.region)++region;auto slots=unsigned(std::floor((region->freqEnd-region->freqStart)/(region->spacing+bwCodeToKHz(plan.lora.bandwidth)/1000.f)));ok=!plan.lora.channel_num || (plan.lora.region && plan.lora.channel_num<=slots);}
        if(ok){CoreSaveGuard guard;channelFile.channels_count=8;for(unsigned i=0;i<8;++i)channelFile.channels[i]=plan.slots[i];if(plan.apply_lora)config.lora=plan.lora;
            // The outer guard suppresses nested checkpoints; publish the complete generation here.
            ok=nodeDB->saveToDisk(SEGMENT_CHANNELS | SEGMENT_CONFIG) && core_storage_commit();if(!ok){channelFile=oldFile;config=oldConfig;core_storage_set_fault();}
            else {++channel_revision;channels.onConfigChanged();service->reloadConfig(0);ok=!error_code && !core_storage_fault();}}
        if(ok)result.kind=MeshEventKind::Configured;else std::snprintf(result.text,sizeof(result.text),"Canais: erro SD/radio/revisao; confira");channel_pending=false;publish(result);
    }
#endif
    MeshCommand c;portENTER_CRITICAL(&queue_mux);bool has=commands.pop(c);portEXIT_CRITICAL(&queue_mux);if(has)execute(c);
    for(unsigned i=0;i<8;++i) {auto p=service->getForPhone();if(!p)break;received(*p);service->releaseToPool(p);}
#ifdef CARDPUTER_ADV_M3
    if(nodes_dirty && !core_storage_fault() && std::uint32_t(millis()-nodes_dirty_at)>=1000) {
        if(!nodeDB->saveToDisk(SEGMENT_NODEDATABASE))core_storage_set_fault();nodes_dirty=false;
    }
#endif
    for(unsigned i=0;i<8;++i) {auto q=service->getQueueStatusForPhone();if(!q)break;if(q->res) {MeshEvent e;e.kind=MeshEventKind::Error;e.packet_id=q->mesh_packet_id;e.result=q->res;std::snprintf(e.text,sizeof(e.text),"Fila radio erro %d",q->res);publish(e);}service->releaseQueueStatusToPool(q);}
    for(unsigned i=0;i<8;++i) {auto n=service->getClientNotificationForPhone();if(!n)break;MeshEvent e;e.kind=MeshEventKind::Error;std::snprintf(e.text,sizeof(e.text),"%s",n->message);publish(e);service->releaseClientNotificationToPool(n);}
    for(unsigned i=0;i<8;++i) {auto m=service->getMqttClientProxyMessageForPhone();if(!m)break;service->releaseMqttClientProxyMessageToPool(m);}
}
MeshSnapshot mesh_snapshot() {
    MeshSnapshot s;s.ready=started&&!core_storage_fault();s.storage_fault=core_storage_fault();s.storage_ready=core_filesystem.begin();
    s.heap_min=heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
#ifdef CARDPUTER_ADV_M8
    s.heap_free=heap_caps_get_free_size(MALLOC_CAP_8BIT);s.loop_max_ms=loop_max_ms;s.uptime_ms=millis();
#endif
    portENTER_CRITICAL(&queue_mux);s.event_drops=event_drops;s.command_drops=command_drops;portEXIT_CRITICAL(&queue_mux);
    if(!started) return s;
    s.node=nodeDB->getNodeNum();s.nodes=nodeDB->getNumMeshNodes();s.region=config.lora.region;s.preset=config.lora.modem_preset;s.frequency_slot=config.lora.channel_num;s.hop_limit=config.lora.hop_limit?config.lora.hop_limit:3;
    s.battery_mv=std::max(0,powerStatus->getBatteryVoltageMv());
#ifdef CARDPUTER_ADV_M8
    // The core already estimates charge from the board's measured voltage.
    auto percent=powerStatus->getBatteryChargePercent();
    s.battery_percent_known=powerStatus->getHasBattery() && percent<=100;
    if(s.battery_percent_known)s.battery_percent=percent;
#endif
    s.radio_ready=RadioLibInterface::instance && !error_code
#ifndef CARDPUTER_ADV_M3
        && !core_storage_fault()
#endif
        ;
    if(auto r=RadioLibInterface::instance) {s.tx=r->txGood;s.rx=r->rxGood;s.rx_errors=r->rxBad;}
    s.utc=getValidTime(RTCQualityGPS);s.gps_time=s.utc!=0;s.gps_fix=gpsStatus->getHasLock();s.satellites=std::min<std::uint32_t>(gpsStatus->getNumSatellites(),255);
    s.latitude=gpsStatus->getLatitude();s.longitude=gpsStatus->getLongitude();s.altitude=gpsStatus->getAltitude();s.gps_fix_millis=gpsStatus->getLastFixMillis();
    std::snprintf(s.channel_name,sizeof(s.channel_name),"%s",channels.getName(channels.getPrimaryIndex()));
    std::memcpy(s.long_name,owner.long_name,sizeof(s.long_name));std::memcpy(s.short_name,owner.short_name,sizeof(s.short_name));return s;
}
std::size_t mesh_channels_page(std::size_t offset,MeshChannel* out,std::size_t capacity) {
    if(!started || !out)return 0;
    auto count=std::min<std::size_t>(channels.getNumChannels(),8);std::size_t n=0;
    while(n<capacity && offset+n<count) {auto index=offset+n;auto& c=channels.getByIndex(index);auto& d=out[n++];d={};d.index=index;d.role=c.role;
        if(c.role!=meshtastic_Channel_Role_DISABLED){std::snprintf(d.name,sizeof(d.name),"%s",channels.getName(index));d.encrypted=channels.storageKey(index).length>0;}
    }
    return n;
}
#ifdef CARDPUTER_ADV_M6
std::uint32_t mesh_channels_revision(){return channel_revision;}
bool mesh_channels_snapshot(ChannelPlan& p){if(!started)return false;p={};p.revision=channel_revision;p.lora=config.lora;for(unsigned i=0;i<8;++i){if(i<channels.getNumChannels())p.slots[i]=channels.getByIndex(i);p.slots[i].index=i;}return channel_plan_valid(p);}
bool mesh_channels_apply(const ChannelPlan& p,std::uint32_t token){if(!token || !started || channel_pending || core_storage_fault() || !RadioLibInterface::instance || (!RadioLibInterface::instance->canSleep() || RadioLibInterface::instance->isSending()) || !channel_plan_valid(p) || p.revision!=channel_revision || commands.size())return false;portENTER_CRITICAL(&queue_mux);bool available=results.reserve();portEXIT_CRITICAL(&queue_mux);if(!available)return false;channel_job=p;channel_token=token;channel_pending=true;return true;}
bool mesh_channels_export(int index,char* url,std::size_t capacity){ChannelPlan p;if(!mesh_channels_snapshot(p))return false;meshtastic_ChannelSet set=meshtastic_ChannelSet_init_zero;auto primary=channels.getPrimaryIndex();for(unsigned n=0;n<8;++n){unsigned i=index>=0?unsigned(index):n?unsigned(n<=primary?n-1:n):primary;if(i>=8)return false;if(p.slots[i].role){set.settings[set.settings_count]=p.slots[i].settings;auto key=channels.storageKey(i);auto& out=set.settings[set.settings_count++];out.psk.size=key.length;if(key.length>0)std::memcpy(out.psk.bytes,key.bytes,key.length);else {out.psk.size=1;out.psk.bytes[0]=0;}out.uplink_enabled=false;out.downlink_enabled=false;}if(index>=0)break;}set.has_lora_config=true;set.lora_config.use_preset=config.lora.use_preset;set.lora_config.modem_preset=config.lora.modem_preset;set.lora_config.channel_num=config.lora.channel_num;return channel_url_encode(set,false,url,capacity);}
#endif
bool mesh_node_name(std::uint32_t number,char* name,std::size_t capacity) {
    if(!started || !name || !capacity)return false;
    auto p=nodeDB->getMeshNode(number);if(!p || !p->has_user || !p->user.short_name[0])return false;
    std::snprintf(name,capacity,"%s",p->user.short_name);return true;
}
std::size_t mesh_nodes_page(std::size_t offset,MeshNode* out,std::size_t cap) {
    if(!started||!out) return 0;cap=std::min<std::size_t>(cap,4);std::uint32_t index=offset;std::size_t n=0;
    while(n<cap) {auto p=nodeDB->readNextMeshNode(index);if(!p)break;auto& d=out[n++];d={};d.number=p->num;d.favorite=p->is_favorite;d.last_heard=p->last_heard;d.snr=p->snr;
        auto hi=std::find(heard.begin(),heard.begin()+heard_count,p->num);if(hi!=heard.begin()+heard_count)d.heard_millis=heard_at[hi-heard.begin()];
        d.has_hops=p->has_hops_away;d.hops=p->hops_away;d.altitude=p->position.altitude;d.position_time=p->position.time;
        if(p->has_device_metrics){d.has_voltage=p->device_metrics.has_voltage && std::isfinite(p->device_metrics.voltage);d.voltage=p->device_metrics.voltage;d.has_battery=p->device_metrics.has_battery_level;d.battery=std::min<std::uint32_t>(p->device_metrics.battery_level,255);}
        d.has_user=p->has_user;d.has_position=p->has_position;d.latitude=p->position.latitude_i;d.longitude=p->position.longitude_i;
        d.heard_this_boot=std::find(heard.begin(),heard.begin()+heard_count,p->num)!=heard.begin()+heard_count;
        d.heard_known=d.heard_this_boot||!heard_overflow;
        if(p->has_user) {std::memcpy(d.long_name,p->user.long_name,sizeof(d.long_name));std::memcpy(d.short_name,p->user.short_name,sizeof(d.short_name));}}
    return n;
}
}
#endif

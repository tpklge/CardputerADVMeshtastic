#ifdef CARDPUTER_ADV_M8
#include "cardputer_adv/maintenance.hpp"
#include "cardputer_adv/core_storage.hpp"
#include <Arduino.h>
namespace cardputer_adv {namespace {
portMUX_TYPE maintenance_mux=portMUX_INITIALIZER_UNLOCKED;
bool initialized=false,pending=false,running=false,completed=false;
MaintenanceResult job,result;
std::uint32_t job_node=0;
char report[1536]{};
void worker(void*){
    for(;;){MaintenanceResult next;std::uint32_t node=0;bool has;
        portENTER_CRITICAL(&maintenance_mux);has=pending;if(has){next=job;node=job_node;pending=false;running=true;}portEXIT_CRITICAL(&maintenance_mux);
        if(has){auto start=millis();
            if(next.action==MaintenanceAction::Diagnostic || next.action==MaintenanceAction::Trace)next.success=core_storage_diagnostic(report,next.filename,sizeof(next.filename),next.action==MaintenanceAction::Trace);
            else if(next.action==MaintenanceAction::Backup)next.success=core_storage_backup(next.slot,node);
            else next.success=core_storage_restore_backup(next.slot,node);
            next.duration_ms=std::uint32_t(millis()-start);
            portENTER_CRITICAL(&maintenance_mux);result=next;completed=true;running=false;portEXIT_CRITICAL(&maintenance_mux);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
}
bool maintenance_begin(){if(initialized)return true;initialized=xTaskCreate(worker,"adv-maint",8192,nullptr,1,nullptr)==pdPASS;return initialized;}
bool maintenance_busy(){portENTER_CRITICAL(&maintenance_mux);bool busy=pending||running||completed;portEXIT_CRITICAL(&maintenance_mux);return busy;}
bool maintenance_submit(MaintenanceAction action,unsigned slot){
    if(!initialized || maintenance_busy())return false;
    bool recovery=action==MaintenanceAction::Restore && core_storage_recovery();
    if(!recovery && (core_storage_fault() || !app_storage_status().ready))return false;
    auto snapshot=mesh_snapshot();if(!recovery && !snapshot.ready)return false;
    if(action==MaintenanceAction::Restore && !recovery && !app_storage_flush(2000))return false;
    if(action==MaintenanceAction::Diagnostic){auto count=diagnostic_text(report,sizeof(report),APP_VERSION,snapshot,app_storage_status());if(count<0 || unsigned(count)>=sizeof(report))return false;}
    if(action==MaintenanceAction::Trace){auto trace=mesh_trace_snapshot();if(!trace.available || trace.tracking)return false;auto count=std::snprintf(report,sizeof(report),"Traceroute !%08lx\n%s\ntruncated=%u\n",static_cast<unsigned long>(trace.node),trace.text,trace.truncated);if(count<0 || unsigned(count)>=sizeof(report))return false;}
    portENTER_CRITICAL(&maintenance_mux);job={};job.action=action;job.slot=slot;job_node=recovery?0:snapshot.node;pending=true;portEXIT_CRITICAL(&maintenance_mux);return true;
}
bool maintenance_poll(MaintenanceResult& out){portENTER_CRITICAL(&maintenance_mux);bool has=completed;if(has){out=result;completed=false;}portEXIT_CRITICAL(&maintenance_mux);return has;}
}
#endif

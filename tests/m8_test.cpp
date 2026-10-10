#include "cardputer_adv/theme.hpp"
#include "cardputer_adv/backup_codec.hpp"
#include "cardputer_adv/maintenance.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <algorithm>
using namespace cardputer_adv;
double luminance(unsigned c){double r=((c>>11)&31)/31.,g=((c>>5)&63)/63.,b=(c&31)/31.;auto linear=[](double v){return v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4);};return .2126*linear(r)+.7152*linear(g)+.0722*linear(b);}
double contrast(unsigned a,unsigned b){auto x=luminance(a),y=luminance(b);return (std::max(x,y)+.05)/(std::min(x,y)+.05);}
int main(){
    for(auto& t:themes){for(auto color:{t.text,t.sent,t.received})for(auto bg:{t.background,t.panel})assert(contrast(color,bg)>=7);assert(contrast(t.active_text,t.active)>=7);assert(t.sent!=t.received);}
    SnapshotManifest m;m.generation=44;for(unsigned i=0;i<5;++i){m.size[i]=i+10;m.crc[i]=i+20;}
    auto w=backup_encode(m,0x1234);SnapshotManifest restored;assert(backup_decode(w,0x1234,restored) && restored.size==m.size && restored.crc==m.crc && restored.generation==44);
    assert(!backup_decode(w,0x5678,restored));for(unsigned i=0;i<w.size();++i){auto bad=w;bad[i]^=1;assert(!backup_decode(bad,0x1234,restored));}
    for(unsigned end=0;end<w.size();++end){BackupWire torn{};std::memcpy(torn.data(),w.data(),end);assert(!backup_decode(torn,0x1234,restored));}
    auto fix=[](BackupWire& v){auto c=store_crc(v.data(),108);for(unsigned i=0;i<4;++i)v[108+i]=c>>(8*i);};
    auto foreign=w;foreign[8]='x';fix(foreign);assert(!backup_decode(foreign,0x1234,restored));auto future=w;future[4]=2;fix(future);assert(!backup_decode(future,0x1234,restored));future=w;future[5]=25;fix(future);assert(!backup_decode(future,0x1234,restored));
    m.size[4]=0;assert(!backup_decode(backup_encode(m,0x1234),0x1234,restored));
    StoredRecord record;record.kind=StoreKind::Ui;record.navigation.shell=true;record.navigation.settings_selection=13;record.navigation.theme=3;std::strcpy(record.event.text,"rascunho preservado");
    StoreWire ui;assert(store_encode(record,ui));assert(ui[321]==11);StoredRecord decoded;assert(store_decode(ui,decoded) && decoded.navigation.theme==3 && decoded.navigation.settings_selection==13 && !std::strcmp(decoded.event.text,record.event.text));
    ui[372]=ui[373]=0;auto crc=store_crc(ui.data(),380);for(unsigned i=0;i<4;++i)ui[380+i]=crc>>(8*i);assert(store_decode(ui,decoded) && decoded.navigation.theme==0 && decoded.navigation.settings_selection==11);
    MeshSnapshot s;s.node=0x1234;s.region=26;s.tx=4;s.rx=8;std::strcpy(s.long_name,"SECRET_OWNER");std::strcpy(s.channel_name,"SECRET_CHANNEL");StorageStatus st;st.ready=true;st.records=77;char text[1536];assert(diagnostic_text(text,sizeof(text),"0.8.0-m8",s,st)>0);
    assert(std::strstr(text,"region=26") && std::strstr(text,"records=77") && !std::strstr(text,"SECRET_") && !std::strstr(text,"psk") && !std::strstr(text,"key") && !std::strstr(text,"meshtastic.org/e/"));
    MeshCommand c;c.kind=MeshCommandKind::TraceRoute;c.destination=0x1234;assert(mesh_validate_command(c));c.destination=0;assert(!mesh_validate_command(c));c.destination=0xffffffff;assert(!mesh_validate_command(c));
}

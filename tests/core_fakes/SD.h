#pragma once
#include <Arduino.h>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <cstring>
constexpr int FILE_READ=0,FILE_WRITE=1,CARD_NONE=0;
inline std::string fail_write_path;
inline int fail_after=-1;
inline std::map<std::string,unsigned> file_writes;
struct File {
    std::shared_ptr<std::vector<std::uint8_t>> bytes;std::string path_;std::size_t at=0;
    explicit operator bool()const{return bool(bytes);}
    bool isDirectory()const{return false;}
    std::size_t size()const{return bytes?bytes->size():0;}
    int read(){std::uint8_t v;return read(&v,1)?v:-1;}
    std::size_t read(std::uint8_t* out,std::size_t count){if(!bytes)return 0;count=std::min(count,bytes->size()-at);std::memcpy(out,bytes->data()+at,count);at+=count;return count;}
    std::size_t write(std::uint8_t v){return write(&v,1);}
    std::size_t write(const std::uint8_t* data,std::size_t count){if(!bytes)return 0;++file_writes[path_];if(path_==fail_write_path && fail_after>=0){count=std::min<std::size_t>(count,fail_after);fail_after-=count;}if(bytes->size()<at+count)bytes->resize(at+count);std::memcpy(bytes->data()+at,data,count);at+=count;return count;}
    void flush(){} void close(){bytes.reset();}
};
struct FakeSD {
    std::map<std::string,std::shared_ptr<std::vector<std::uint8_t>>> files;std::set<std::string> dirs;
    bool exists(const String& path)const{return files.count(path) || dirs.count(path);}
    bool mkdir(const String& path){dirs.insert(path);return true;}
    bool remove(const String& path){return files.erase(path);}
    bool rename(const String& a,const String& b){if(!files.count(a) || exists(b))return false;files[b]=files.at(a);files.erase(a);return true;}
    File open(const String& path,int mode){if(mode==FILE_READ){auto i=files.find(path);return i==files.end()?File{}:File{i->second,path,0};}auto& f=files[path];f=std::make_shared<std::vector<std::uint8_t>>();return {f,path,0};}
    bool begin(int,int,int,const char*,int,bool){return true;}
    int cardType(){return 1;}
    unsigned long totalBytes(){return 64*1024*1024;}unsigned long usedBytes(){return 100000;}
};
inline FakeSD SD;

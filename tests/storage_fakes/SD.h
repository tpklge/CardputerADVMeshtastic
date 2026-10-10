#pragma once
#include <Arduino.h>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <cstring>
constexpr int FILE_READ=0,FILE_WRITE=1,FILE_APPEND=2;
struct FakeFile {
    std::shared_ptr<std::vector<std::uint8_t>> bytes;
    std::size_t at=0;
    explicit operator bool()const{return bool(bytes);}
    bool seek(std::size_t offset){if(!bytes || offset>bytes->size())return false;at=offset;return true;}
    std::size_t size()const{return bytes?bytes->size():0;}
    std::size_t read(std::uint8_t* out,std::size_t count){if(!bytes)return 0;count=std::min(count,bytes->size()-at);std::memcpy(out,bytes->data()+at,count);at+=count;return count;}
    std::size_t write(const std::uint8_t* data,std::size_t count){if(!bytes)return 0;if(bytes->size()<at+count)bytes->resize(at+count);std::memcpy(bytes->data()+at,data,count);at+=count;return count;}
    void flush(){} void close(){}
};
using File=FakeFile;
struct FakeSD {
    std::map<std::string,std::shared_ptr<std::vector<std::uint8_t>>> files;
    std::set<std::string> dirs;
    bool exists(const String& path)const{return files.count(path) || dirs.count(path);}
    bool mkdir(const String& path){dirs.insert(path);return true;}
    bool remove(const String& path){return files.erase(path);}
    FakeFile open(const String& path,int mode){if(mode==FILE_READ){auto i=files.find(path);return i==files.end()?FakeFile{}:FakeFile{i->second,0};}auto& f=files[path];if(!f || mode==FILE_WRITE)f=std::make_shared<std::vector<std::uint8_t>>();return {f,mode==FILE_APPEND?f->size():0};}
};
inline FakeSD SD;

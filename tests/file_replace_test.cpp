#include "cardputer_adv/file_replace.hpp"
#include <cassert>
#include <map>
#include <string>
struct Fat {
    std::map<std::string,int> files;
    int calls=0,fail=0;
    bool exists(const std::string& p) { return files.count(p); }
    bool rename(const std::string& a,const std::string& b) {
        if(++calls==fail || !exists(a) || exists(b)) return false;
        files[b]=files[a];files.erase(a);return true;
    }
    bool remove(const std::string& p) {return files.erase(p)==1;}
};
int main() {
    const std::string tmp="tmp",target="config",backup="bak";
    Fat fs{{{tmp,2},{target,1}}};
    assert(cardputer_adv::replace_file(fs,tmp,target,backup));
    assert(fs.files.at(target)==2 && !fs.exists(backup));
    for(int fail: {1,2}) {
        Fat broken{{{tmp,2},{target,1}},0,fail};
        assert(!cardputer_adv::replace_file(broken,tmp,target,backup));
        assert(broken.files.at(target)==1 && broken.files.at(tmp)==2);
    }
    Fat pending{{{tmp,2},{backup,1}}};
    assert(!cardputer_adv::replace_file(pending,tmp,target,backup));
    assert(pending.files.at(backup)==1 && pending.files.at(tmp)==2);
    Fat fresh{{{tmp,2}}};
    assert(cardputer_adv::replace_file(fresh,tmp,target,backup));
    assert(fresh.files.at(target)==2);
}

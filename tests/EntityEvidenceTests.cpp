#include "EntityEvidence.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>
#include <iostream>
struct FakeMemory {
    std::map<uint32_t,uint8_t> bytes;
    void put(uint32_t address,const std::vector<uint32_t>& words) {
        for(size_t i=0;i<words.size();++i) for(uint32_t b=0;b<4;++b)
            bytes[address+static_cast<uint32_t>(i*4+b)]=static_cast<uint8_t>(words[i]>>(b*8));
    }
    bool read(uint32_t address,void* dst,size_t size) const {
        auto* output=static_cast<uint8_t*>(dst);
        for(size_t i=0;i<size;++i) if(!bytes.count(address+static_cast<uint32_t>(i)))return false;
        for(size_t i=0;i<size;++i)output[i]=bytes.at(address+static_cast<uint32_t>(i));
        return true;
    }
};
int main(){
    using namespace aoe2hd;
    FakeMemory mem;
    // two entries, one local and one enemy, with a repeated indirect reference
    mem.put(0x20000,{0x30000,0x40000});
    mem.put(0x30000,{0x50000,0,22,0x70000,0x80000});
    mem.put(0x40000,{0x60000,0,23,0x70000,0x90000});
    mem.put(0x50000,{0,0,32,0,0x80000});
    mem.put(0x60000,{0,0,33,0,0x90000});
    auto reader=[&](uint32_t a,void* d,size_t n){return mem.read(a,d,n);};
    auto out=inspect(reader,0x20000,4,2,{0xA0000,0x80000,0x90000},1);
    bool local=false,enemy=false,indirect=false;
    for(const auto& e:out){
        if(e.path==std::string("direct") && e.target==0x30000){
            assert(e.readable && e.candidateId==22);
            assert(std::string(e.ownership)=="local");local=true;
        }
        if(e.path==std::string("direct") && e.target==0x40000){
            assert(e.readable && e.candidateId==23);
            assert(std::string(e.ownership)=="other_player");enemy=true;
        }
        if(e.target==0x50000 && std::string(e.path)=="indirect_0")indirect=true;
    }
    assert(local && enemy && indirect);
    assert(inspect(reader,0xFFFFFFFC,4,2,{0x80000},0).empty());
    assert(inspect(reader,0x20000,3,2,{0x80000},0).empty());
    assert(inspect(reader,0x20000,4,513,{0x80000},0).empty());
    assert(std::string(classify(0x123400,{0x80000},0))=="unmatched");
    assert(std::string(classify(0,{0x80000},0))=="unknown");
    mem.put(0x22000,{0x30000,0x30000});
    auto duplicates=inspect(reader,0x22000,4,2,{0x80000},0);
    bool duplicate=false;
    for(const auto& e:duplicates) if(e.target==0x30000 && e.duplicate)duplicate=true;
    assert(duplicate);
    std::cout<<"EntityEvidenceTests: OK\n";
}

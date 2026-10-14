#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace aoe2hd {
struct Evidence {
    uint32_t index=0, word=0, value=0, target=0;
    uint32_t signature=0, candidateId=0, candidateType=0, candidateOwner=0;
    const char* path="none";
    const char* ownership="unknown";
    bool readable=false;
    bool duplicate=false;
};
inline bool validPtr(uint32_t p) { return p>=0x10000u && !(p&3u) && p<=UINT32_MAX-20u; }
inline const char* classify(uint32_t owner,const std::vector<uint32_t>& players,uint32_t local) {
    if (!owner) return "unknown";
    if(local<players.size() && owner==players[local]) return "local";
    for(size_t i=0;i<players.size();++i)
        if(i!=local && players[i]==owner) return "other_player";
    return "unmatched";
}
// Read callback: bool(uint32_t address, void* destination, size_t bytes).
// First 20 bytes follow the v0 experimental layout:
// +0 signature, +8 candidate ID, +12 type reference, +16 owner.
template<class Reader>
std::vector<Evidence> inspect(Reader&& read,uint32_t begin,uint32_t stride,uint32_t count,
                              const std::vector<uint32_t>& players,uint32_t local,
                              uint32_t maxSamples=64) {
    std::vector<Evidence> out;
    if(!validPtr(begin) || (stride!=4 && stride!=8) || count>512 || !count)return out;
    std::set<uint32_t> unique;
    for(uint32_t i=0;i<std::min(count,maxSamples);++i) {
        const uint64_t addr=uint64_t(begin)+uint64_t(i)*stride;
        if(addr>UINT32_MAX-8)break;
        for(uint32_t word=0;word<stride/4;++word) {
            uint32_t value=0;
            if(!read(static_cast<uint32_t>(addr+word*4),&value,4))continue;
            Evidence direct;
            direct.index=i;direct.word=word;direct.value=value;direct.target=value;
            direct.path="direct";
            if(!validPtr(value)) {out.push_back(direct);continue;}
            uint32_t identity[5]{};
            if(read(value,identity,sizeof(identity))) {
                direct.readable=true;direct.signature=identity[0];
                direct.candidateId=identity[2];direct.candidateType=identity[3];
                direct.candidateOwner=identity[4];
                direct.ownership=classify(direct.candidateOwner,players,local);
                direct.duplicate=!unique.insert(value).second;
            }
            out.push_back(direct);
            // Probe first two words of an intermediate node, with strict one-hop cap.
            for(uint32_t slot=0;slot<2;++slot) {
                uint32_t target=0;
                if(!read(value+slot*4,&target,4) || !validPtr(target))continue;
                Evidence indirect;
                indirect.index=i;indirect.word=word;indirect.value=value;indirect.target=target;
                indirect.path=slot==0?"indirect_0":"indirect_4";
                if(read(target,identity,sizeof(identity))) {
                    indirect.readable=true;indirect.signature=identity[0];
                    indirect.candidateId=identity[2];indirect.candidateType=identity[3];
                    indirect.candidateOwner=identity[4];
                    indirect.ownership=classify(indirect.candidateOwner,players,local);
                    indirect.duplicate=!unique.insert(target).second;
                }
                out.push_back(indirect);
            }
        }
    }
    return out;
}
} // namespace aoe2hd

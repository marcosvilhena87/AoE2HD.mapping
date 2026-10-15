#include "BuildingObjectDiscoveryV4.h"
#include <cassert>
int main(){
 std::vector<uint8_t> bytes={0x10,0,0,0,0x20,0,0,0};
 auto words=aoe2hd::DecodeWindowV4(bytes);
 assert(words.size()==2&&words[0].offset==0&&words[0].value==16);
 assert(words[1].offset==4&&words[1].value==32);
 assert(aoe2hd::IsCandidateObjectV4(0x20000,2183,2183));
 assert(!aoe2hd::IsCandidateObjectV4(0x20001,2183,2183));
}

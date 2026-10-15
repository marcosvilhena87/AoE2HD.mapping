#include "PlayerObjectCollectionDiscoveryV5.h"
#include <cassert>
#include <cstring>
int main(){
 std::vector<uint8_t> b(48,0);
 auto put=[&](size_t off,uint32_t v){std::memcpy(b.data()+off,&v,4);};
 put(0,0x20000);put(4,0x30000);put(8,0x40000);put(12,0x50000);
 put(20,0x60000);put(24,0x60010);put(28,0x60020);
 auto probe=[](uint32_t v){return v>=0x20000&&v<=0x60020;};
 auto readId=[](uint32_t v,uint32_t& id){id=(v==0x20000)?2183:2499;return true;};
 auto m=aoe2hd::ExamineCollectionV5(b,{{0x20000,2183},{0x30000,2491}},probe,readId);
 assert(m.longestPointerRun==4&&m.anchorMatches==2&&m.distinctAnchors==2);
 assert(m.beginEndCapacityTriples>=1&&m.validatedObjectIDs>=2);
 auto zero=aoe2hd::ExamineCollectionV5(std::vector<uint8_t>(16),{},probe,readId);
 assert(zero.score==0&&zero.distinctAnchors==0);
}

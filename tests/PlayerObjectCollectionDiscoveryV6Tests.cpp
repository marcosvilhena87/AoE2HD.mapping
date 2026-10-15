#include "PlayerObjectCollectionDiscoveryV6.h"
#include <cassert>
#include <cstring>
int main(){
 std::vector<uint8_t> b(32,0);
 auto put=[&](size_t i,uint32_t value){std::memcpy(b.data()+i,&value,4);};
 put(0,0x20000);put(4,0x30000);put(8,0x40000);
 auto validate=[](uint32_t p,uint32_t& id){if(p==0x20000||p==0x30000){id=2183;return true;}return false;};
 auto m=aoe2hd::InspectTargetV6(b,{{0x20000,2183},{0x30000,2491}},validate);
 assert(m.anchorMatches==2&&m.distinctAnchors==2);
 assert(m.longestRun==3&&m.readableIds==2&&m.plausibleIds==2);
 auto z=aoe2hd::InspectTargetV6(std::vector<uint8_t>(32,0),{},validate);
 assert(z.pointerSlots==0&&z.anchorMatches==0&&z.score==0);
 assert(z.fingerprint!=m.fingerprint);
}

#include "BuildingObjectDiscoveryV7.h"
#include <cassert>
int main(){
 std::vector<uint8_t> bytes={0x00,0x00,0x02,0x00,0x00,0x00,0x03,0x00,0x00,0,0,0};
 std::vector<aoe2hd::ObjectIdentityV7> anchors={{0x20000,2183},{0x30000,2491}};
 auto hits=aoe2hd::MatchKnownObjectsV7(0x10000,bytes,anchors);
 assert(hits.size()==2&&hits[0].slot==0x10000&&hits[0].matched==0);
 assert(hits[1].slot==0x10004&&hits[1].matched==1);
 assert(aoe2hd::MatchKnownObjectsV7(0x10000,bytes,{{0x40000,1}}).empty());
}

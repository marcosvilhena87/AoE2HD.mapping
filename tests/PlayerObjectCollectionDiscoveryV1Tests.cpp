#include "PlayerObjectCollectionDiscoveryV1.h"
#include <cassert>
int main(){
 std::vector<uint8_t> b={0,0,2,0,0,0,3,0,0,0,2,0};
 std::vector<aoe2hd::ObjectAnchorV1> a={{0x20000,1},{0x30000,2},{0x40000,3}};
 auto e=aoe2hd::AnalyzeCandidateV1(b,a);
 assert(e.matches==3&&e.distinct==2&&e.mask==3);
 assert(aoe2hd::AnalyzeCandidateV1({},a).distinct==0);
}

#include "BuildingObjectDiscoveryV2.h"
#include <cassert>
int main(){
 const uint8_t values[]={0x10,0,0x02,0,0x20,0,0x02,0,0,0,0,0};
 auto hits=aoe2hd::ScanDualBuildingHitsV2(0x10000,values,sizeof(values),0x20010,0x20020);
 assert(hits.size()==2&&hits[0].address==0x10000&&hits[1].address==0x10004);
 assert(aoe2hd::HasBothAnchorsV2(hits,0x20010,0x20020));
 assert(!aoe2hd::HasBothAnchorsV2(hits,0x20010,0x20030));
}

#include "BuildingReferenceDiscoveryV1.h"
#include <cassert>
#include <cstdint>
int main(){
 const uint8_t bytes[]={0x34,0x12,0,0,0,0,0,0,0x34,0x12,0,0};
 auto hits=aoe2hd::FindDirectReferencesV1(0x10000,bytes,sizeof(bytes),0x1234);
 assert(hits.size()==2&&hits[0].address==0x10000&&hits[1].address==0x10008);
 assert(aoe2hd::PersistentReferenceV1(0x1234,0x1234,0x1234));
 assert(!aoe2hd::PersistentReferenceV1(0x1234,0x9876,0x1234));
}

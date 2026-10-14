#include "AnchorPersistenceScan.h"
#include <cassert>
#include <cstdint>
int main(){
 const uint32_t a=0x110000,b=0x220000;
 uint32_t data[]={a,b,a,b};
 std::vector<aoe2hd::AnchorReference> first,second;
 aoe2hd::CollectAnchorReferences(0x30000,0x30000,
  reinterpret_cast<const uint8_t*>(data),sizeof(data),a,b,first,second,1);
 assert(first.size()==1&&second.size()==1);
 assert(first[0].address==0x30000&&second[0].address==0x30004);
 assert(aoe2hd::ClassifyReference(true,a,a,b)==aoe2hd::Persistence::first);
 assert(aoe2hd::ClassifyReference(true,b,a,b)==aoe2hd::Persistence::second);
 assert(aoe2hd::ClassifyReference(true,9,a,b)==aoe2hd::Persistence::other);
 assert(aoe2hd::ClassifyReference(false,0,a,b)==aoe2hd::Persistence::unreadable);
}
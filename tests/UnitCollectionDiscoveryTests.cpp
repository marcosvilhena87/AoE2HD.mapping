#include "UnitCollectionDiscovery.h"
#include <cassert>
#include <map>
#include <cstring>
int main(){
 std::map<uint32_t,uint32_t> mem{
 {0x20000,0x11110000},{0x20004,0x30000},
 {0x20008,0x40000},{0x2000c,0},
 {0x30000,0x11110000},{0x30004,0},
 {0x40000,0x22220000},{0x40004,0}
 };
 auto read=[&](uint32_t address,void* out,size_t n){
  if(n!=4||!mem.count(address))return false;
  uint32_t v=mem[address];std::memcpy(out,&v,4);return true;
 };
 auto e=aoe2hd::InspectCollection(read,0x88,0x20000,2,8,0x11110000,0x22220000);
 assert(aoe2hd::HasBoth(e));assert(e.directFirst==1);
 assert(e.indirectFirst>=1&&e.indirectSecond>=1);
 assert(e.firstIndex==0&&e.secondIndex==1);
 auto bad=aoe2hd::InspectCollection(read,0x88,0x20000,2,8,0x11110000,0x33330000);
 assert(!aoe2hd::HasBoth(bad));
 assert(!aoe2hd::HasBoth(aoe2hd::InspectCollection(read,0,0x20000,513,8,1,2)));
 assert(aoe2hd::IsSelectionLike(0x15c));
 assert(!aoe2hd::IsSelectionLike(0x88));
}
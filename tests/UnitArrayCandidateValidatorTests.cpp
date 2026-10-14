#include "UnitArrayCandidateValidator.h"
#include <cassert>
#include <cstring>
#include <map>
int main(){
 auto pairs=aoe2hd::FindEightBytePairs({0x20000,0x40000},{0x20008,0x40020});
 assert(pairs.size()==1&&pairs[0].firstSlot==0x20000&&pairs[0].secondSlot==0x20008);
 std::map<uint32_t,uint32_t> m{{0x20000,0x30000},{0x20004,0x55},
  {0x30008,2044},{0x30034,0x41c80000}};
 auto rd=[&](uint32_t at,void* dst,size_t n){
  auto it=m.find(at);if(n!=4||it==m.end())return false;
  std::memcpy(dst,&it->second,4);return true;
 };
 auto entry=aoe2hd::InspectEntry(rd,0x20000);
 assert(entry.plausible&&entry.id==2044&&entry.hp==25.0f&&entry.meta==0x55);
 assert(!aoe2hd::InspectEntry(rd,0x21000).plausible);
}
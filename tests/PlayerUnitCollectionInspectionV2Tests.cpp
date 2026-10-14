#include "PlayerUnitCollectionInspectionV2.h"
#include <cassert>
#include <cstring>
#include <map>
int main(){
 std::map<uint32_t,uint32_t> memory{
  {0x20000,2044},{0x20004,0x30000},
  {0x30000,0x40000},{0x40008,2044},{0x40034,0x41c80000}
 };
 auto read=[&](uint32_t at,void* out,size_t n){
  auto it=memory.find(at);if(it==memory.end()||n!=4)return false;
  std::memcpy(out,&it->second,4);return true;
 };
 auto direct=aoe2hd::InspectPlayerEntry(read,0x20000,0,0,2044,2045);
 assert(direct.raw==2044&&direct.direct&&!direct.plausible);
 auto indirect=aoe2hd::InspectPlayerEntry(read,0x20000,0,4,0x40000,0x50000);
 assert(indirect.viaIndirect&&indirect.plausible&&indirect.id==2044&&indirect.hp==25.0f);
}
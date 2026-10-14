#include "EntityGraphCoverageV5.h"
#include <cassert>
#include <cstring>
#include <map>
int main(){
 std::map<uint32_t,uint32_t> m{{0x10184,0x20000},{0x10188,0x20010},
   {0x20000,0x30000},{0x20004,0},{0x20008,0x40000},{0x2000c,0}};
 auto reader=[&](uint32_t at,void* dst,size_t n){
  auto it=m.find(at);if(n!=4||it==m.end())return false;
  std::memcpy(dst,&it->second,4);return true;
 };
 auto roots=aoe2hd::ReadPlayerRoots(reader,0x10000);
 assert(roots.size()==2&&roots[0]==0x30000&&roots[1]==0x40000);
 assert(aoe2hd::IsStableReference(true,12,12));
 assert(!aoe2hd::IsStableReference(true,12,13));
 assert(!aoe2hd::IsStableReference(false,12,12));
 assert(aoe2hd::IsSwitchingReference(true,20,10,10,20));
 assert(!aoe2hd::IsSwitchingReference(true,20,20,10,20));
 assert(aoe2hd::BothAnchorsObserved(1,2));
 assert(!aoe2hd::BothAnchorsObserved(0,2));
 return 0;
}
#include "PlayerObjectClassificationV3.h"
#include <cassert>
#include <cstring>
#include <map>
int main(){
 std::map<uint32_t,uint32_t> mem{{0x20000,0x70000},{0x20008,2044},
 {0x20010,42},{0x20034,0x41c80000},{0x30000,0x70000},
 {0x30008,2045},{0x30010,42},{0x30034,0x41c80000},
 {0x40000,0x70000},{0x40008,2046},{0x40010,42},{0x40034,0x41c80000}};
 auto reader=[&](uint32_t addr,void* out,size_t n){
  auto it=mem.find(addr);if(it==mem.end()||n!=4)return false;
  std::memcpy(out,&it->second,4);return true;
 };
 auto a=aoe2hd::FingerprintObject(reader,0x20000);
 auto b=aoe2hd::FingerprintObject(reader,0x30000);
 auto c=aoe2hd::FingerprintObject(reader,0x40000);
 assert(a.readable&&a.hpSane&&a.id==2044);
 assert(std::strcmp(aoe2hd::ClassifyObject(c,a,b),"anchor_vtable_match")==0);
 assert(std::strcmp(aoe2hd::ClassifyObject(a,a,b),"known_anchor")==0);
 assert(!aoe2hd::FingerprintObject(reader,0x12345).readable);
}
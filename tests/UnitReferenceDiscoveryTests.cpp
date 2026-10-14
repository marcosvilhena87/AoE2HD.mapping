#include "UnitReferenceDiscovery.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <iostream>
int main(){
 std::map<uint32_t,uint32_t> words={{0x20000,0x50000},{0x20004,0x30000},
 {0x20008,0x40000},{0x2000c,0x50000},{0x30000,0x50000},
 {0x40000,0x1234},{0x40004,0x50000}};
 auto read=[&](uint32_t addr,void* dst,size_t n){
  if(n!=4||!words.count(addr))return false;
  auto value=words[addr];std::memcpy(dst,&value,4);return true;
 };
 auto hits=aoe2hd::FindReferences(read,0x20000,16,0x50000,"World");
 size_t direct=0,indirect0=0,indirect4=0;
 for(const auto& h:hits){
  if(std::string(h.path)=="direct")++direct;
  if(std::string(h.path)=="indirect_0")++indirect0;
  if(std::string(h.path)=="indirect_4")++indirect4;
 }
 assert(direct==2&&indirect0==1&&indirect4==1);
 assert(aoe2hd::FindReferences(read,0x20000,8196,0x50000,"bad").empty());
 assert(aoe2hd::FindReferences(read,0xFFFFFFFC,16,0x50000,"bad").empty());
 assert(aoe2hd::FindReferences(read,0x20000,16,0,"bad").empty());
 assert(aoe2hd::FindReferences(read,0x20000,16,0x50000,"ok",1).size()==1);
 std::cout<<"UnitReferenceDiscoveryTests OK\n";
}

#include "StructuralGraphEvidence.h"
#include <cassert>
#include <cstring>
#include <map>
int main(){
 std::map<uint32_t,uint32_t> memory{{0x20000,7}};
 auto rd=[&](uint32_t address,void* dst,size_t n){
  auto i=memory.find(address);if(i==memory.end()||n!=4)return false;
  std::memcpy(dst,&i->second,4);return true;
 };
 assert(aoe2hd::ValidateReadablePointer(rd,0x20000));
 assert(!aoe2hd::ValidateReadablePointer(rd,0x30000));
 assert(!aoe2hd::ValidateReadablePointer(rd,0x12));
 uint32_t bytes[4]={0x20000,0x30000,0x20000,0};
 auto hits=aoe2hd::FindReverseInBuffer(0x1015c,
  reinterpret_cast<const uint8_t*>(bytes),sizeof(bytes),{0x20000},0x10000);
 assert(hits.size()==2&&hits[0].selection&&hits[1].address==0x10164);
 assert(aoe2hd::FindReverseInBuffer(0x1015c,
  reinterpret_cast<const uint8_t*>(bytes),sizeof(bytes),{0x20000},0x10000,1).size()==1);
}
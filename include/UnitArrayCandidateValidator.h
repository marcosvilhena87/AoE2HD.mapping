#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>
namespace aoe2hd {
struct ArrayPair {uint32_t firstSlot=0,secondSlot=0,stride=0;};
inline std::vector<ArrayPair> FindEightBytePairs(
 const std::vector<uint32_t>& first,const std::vector<uint32_t>& second,
 size_t limit=256){
 std::vector<ArrayPair> out;
 for(uint32_t a:first)for(uint32_t b:second){
  const uint64_t distance=a>b?uint64_t(a)-b:uint64_t(b)-a;
  if(distance==8 && out.size()<limit)
   out.push_back({a,b,8});
 }
 return out;
}
struct EntrySnapshot {uint32_t slot=0,value=0,meta=0,id=0;float hp=0;bool plausible=false;};
template<class Read>
EntrySnapshot InspectEntry(Read&& read,uint32_t slot){
 EntrySnapshot e;e.slot=slot;
 if(!read(slot,&e.value,4)||!read(slot+4,&e.meta,4)||
    e.value<0x10000||(e.value&3u)||e.value>UINT32_MAX-0x38)return e;
 uint32_t bits=0;
 if(!read(e.value+8,&e.id,4)||!read(e.value+0x34,&bits,4))return e;
 std::memcpy(&e.hp,&bits,4);
 // Reject NaN/inf without platform-dependent float helpers.
 e.plausible=((bits&0x7f800000u)!=0x7f800000u)&&e.hp>=0&&e.hp<=100000&&e.id>0;
 return e;
}
}
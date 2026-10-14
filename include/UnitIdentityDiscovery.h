#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>
namespace aoe2hd {
struct IdentityHit {
 uint32_t offset=0, width=0, value=0, recordStride=0, recordIndex=0, fieldOffset=0;
};
// Inspect all DWORD-aligned ID occurrences, including a one-DWORD neighbor
// that might be an object pointer. No proof of an entity table is implied.
inline std::vector<IdentityHit> FindIdentityHits(const uint8_t* data,size_t size,
 uint32_t idA,uint32_t idB,uint32_t stride,size_t limit=4096){
 std::vector<IdentityHit> out;
 if(!data||!limit||!idA||!idB||idA==idB||stride<4||stride%4)return out;
 for(size_t pos=0;pos+4<=size && out.size()<limit;pos+=4){
  uint32_t value=0;std::memcpy(&value,data+pos,4);
  if(value!=idA&&value!=idB)continue;
  out.push_back({static_cast<uint32_t>(pos),4,value,stride,
   static_cast<uint32_t>(pos/stride),static_cast<uint32_t>(pos%stride)});
 }
 return out;
}
inline bool BothIds(const std::vector<IdentityHit>& hits,uint32_t a,uint32_t b){
 bool first=false,second=false;
 for(const auto& h:hits){first|=h.value==a;second|=h.value==b;}
 return first&&second;
}
}
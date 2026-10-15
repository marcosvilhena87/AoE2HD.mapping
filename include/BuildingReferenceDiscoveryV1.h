#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
namespace aoe2hd {
struct ReferenceHitV1 {uint32_t address=0, target=0;};
inline std::vector<ReferenceHitV1> FindDirectReferencesV1(uint32_t base,const uint8_t* data,size_t size,uint32_t target){
 std::vector<ReferenceHitV1> hits;
 for(size_t i=0;i+4<=size;i+=4){
  uint32_t value=0;std::memcpy(&value,data+i,4);
  if(value==target&&uint64_t(base)+i<=UINT32_MAX)
   hits.push_back({uint32_t(base+i),target});
 }
 return hits;
}
inline bool PersistentReferenceV1(uint32_t initial,uint32_t later,uint32_t target){
 return initial==target&&later==target;
}
} // namespace aoe2hd

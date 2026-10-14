#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
namespace aoe2hd {
struct AnchorReference {uint32_t address=0,value=0,region=0;};
inline void CollectAnchorReferences(uint32_t base,uint32_t region,
 const uint8_t* bytes,size_t size,uint32_t first,uint32_t second,
 std::vector<AnchorReference>& firstOut,std::vector<AnchorReference>& secondOut,
 size_t perAnchorLimit){
 if(!bytes||!first||!second||first==second)return;
 for(size_t i=0;i+4<=size;i+=4){
  const uint64_t address=uint64_t(base)+i;
  if(address>UINT32_MAX-3)break;
  uint32_t value=0;std::memcpy(&value,bytes+i,4);
  if(value==first&&firstOut.size()<perAnchorLimit)
   firstOut.push_back({uint32_t(address),value,region});
  if(value==second&&secondOut.size()<perAnchorLimit)
   secondOut.push_back({uint32_t(address),value,region});
 }
}
enum class Persistence {unreadable, first, second, other};
inline Persistence ClassifyReference(bool readable,uint32_t value,
 uint32_t first,uint32_t second){
 if(!readable)return Persistence::unreadable;
 if(value==first)return Persistence::first;
 if(value==second)return Persistence::second;
 return Persistence::other;
}
}
#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
namespace aoe2hd {
struct DualBuildingHitV2 {uint32_t address=0;uint32_t target=0;};
inline std::vector<DualBuildingHitV2> ScanDualBuildingHitsV2(uint32_t base,const uint8_t* data,size_t bytes,uint32_t a,uint32_t b){
 std::vector<DualBuildingHitV2> hits;
 for(size_t i=0;i+4<=bytes;i+=4){
  uint32_t v=0;std::memcpy(&v,data+i,4);
  if((v==a||v==b)&&uint64_t(base)+i<=UINT32_MAX)hits.push_back({uint32_t(base+i),v});
 }
 return hits;
}
inline bool HasBothAnchorsV2(const std::vector<DualBuildingHitV2>& v,uint32_t a,uint32_t b){
 bool foundA=false,foundB=false;for(const auto& hit:v){foundA|=hit.target==a;foundB|=hit.target==b;}return foundA&&foundB;
}
} // namespace aoe2hd

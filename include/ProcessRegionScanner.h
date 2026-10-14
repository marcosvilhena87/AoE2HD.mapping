#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
namespace aoe2hd {
struct RegionMatch {uint32_t address=0;uint32_t before=0;uint32_t after=0;};
// Pure buffer search. Only complete aligned DWORD positions are considered.
// Addresses are 32-bit TARGET addresses, independent of host architecture.
inline std::vector<RegionMatch> SearchPointerChunk(uint32_t base,const uint8_t* data,
 size_t length,uint32_t anchor,uint32_t maxHits=1000) {
 std::vector<RegionMatch> out;
 if(!data||!anchor||!maxHits)return out;
 for(size_t i=0;i+4<=length;++i){
  uint64_t address=uint64_t(base)+i;
  if(address>UINT32_MAX-3)break;
  if(address%4)continue;
  uint32_t value=0;std::memcpy(&value,data+i,4);
  if(value!=anchor)continue;
  uint32_t before=0,after=0;
  if(i>=4)std::memcpy(&before,data+i-4,4);
  if(i+8<=length)std::memcpy(&after,data+i+4,4);
  out.push_back({static_cast<uint32_t>(address),before,after});
  if(out.size()>=maxHits)break;
 }
 return out;
}
} // namespace aoe2hd

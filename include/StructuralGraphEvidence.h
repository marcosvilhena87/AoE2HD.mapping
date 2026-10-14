#pragma once
#include <cstdint>
#include <cstring>
#include <set>
#include <vector>
namespace aoe2hd {
struct StructuralHit {uint32_t address=0,target=0;bool selection=false;};
inline bool LooksLikeAddress(uint32_t x){return x>=0x10000 && !(x&3u);}
template<class Reader>
bool ValidateReadablePointer(Reader&& read,uint32_t value){
 if(!LooksLikeAddress(value))return false;
 uint32_t word=0;return read(value,&word,4);
}
inline std::vector<StructuralHit> FindReverseInBuffer(uint32_t base,
 const uint8_t* data,size_t size,const std::set<uint32_t>& targets,
 uint32_t local,uint32_t maxHits=4096){
 std::vector<StructuralHit> result;
 if(!data||!maxHits)return result;
 for(size_t i=0;i+4<=size&&result.size()<maxHits;i+=4){
  const uint64_t addr=uint64_t(base)+i;
  if(addr>UINT32_MAX-3)break;
  uint32_t word=0;std::memcpy(&word,data+i,4);
  if(targets.count(word))
   result.push_back({uint32_t(addr),word,
    addr==uint64_t(local)+0x15c||addr==uint64_t(local)+0x160});
 }
 return result;
}
}
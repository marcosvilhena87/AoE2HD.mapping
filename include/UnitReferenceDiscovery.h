#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
namespace aoe2hd {
struct ReferenceHit {
 uint32_t address=0, value=0, intermediate=0;
 uint32_t offset=0, index=0, word=0;
 const char* source=""; const char* path="";
};
// Scans only supplied bounded, readable regions; one indirection maximum.
// The value at a direct slot equals anchor; indirect means [value]==anchor
// or [value+4]==anchor. The scanner never follows the anchor itself.
template<class Reader>
std::vector<ReferenceHit> FindReferences(Reader&& read,uint32_t base,uint32_t bytes,
                                         uint32_t anchor,const char* source,
                                         uint32_t maxHits=2048) {
 std::vector<ReferenceHit> out;
 if(!base||!anchor||bytes>8192||base>UINT32_MAX-bytes||
    anchor<0x10000||(anchor&3u))return out;
 for(uint32_t off=0;off+4<=bytes&&out.size()<maxHits;off+=4){
  uint32_t v=0;
  if(!read(base+off,&v,4))continue;
  if(v==anchor)out.push_back({base+off,v,0,off,off/4,0,source,"direct"});
  if(v<0x10000||(v&3u)||v>UINT32_MAX-8||v==anchor)continue;
  for(uint32_t slot=0;slot<2&&out.size()<maxHits;++slot){
   uint32_t target=0;
   if(read(v+slot*4,&target,4)&&target==anchor)
    out.push_back({base+off,v,v,off,off/4,slot,source,slot?"indirect_4":"indirect_0"});
  }
 }
 return out;
}
} // namespace aoe2hd

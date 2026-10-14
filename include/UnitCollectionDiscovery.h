#pragma once
#include <cstdint>
#include <vector>
#include <algorithm>
namespace aoe2hd {
struct CollectionEvidence {
 uint32_t rootOffset=0,buffer=0,stride=0,count=0;
 uint32_t firstIndex=UINT32_MAX,secondIndex=UINT32_MAX;
 uint32_t directFirst=0,directSecond=0,indirectFirst=0,indirectSecond=0;
};
// A candidate is supported only when BOTH distinct anchors occur in entries.
// A word may point directly to an anchor, or to a readable holder whose
// first or second DWORD points to it. This never confirms a unit list.
template<class Read>
CollectionEvidence InspectCollection(Read&& read,uint32_t rootOffset,
 uint32_t buffer,uint32_t count,uint32_t stride,uint32_t first,uint32_t second){
 CollectionEvidence e;e.rootOffset=rootOffset;e.buffer=buffer;e.count=count;e.stride=stride;
 if(!buffer||count>512||(stride!=4&&stride!=8)||!first||!second||first==second)return e;
 for(uint32_t i=0;i<count;++i){
  for(uint32_t w=0;w<stride/4;++w){
   uint64_t slot=uint64_t(buffer)+uint64_t(i)*stride+w*4;
   if(slot>UINT32_MAX-4)continue;
   uint32_t v=0;if(!read(uint32_t(slot),&v,4))continue;
   if(v==first){++e.directFirst;e.firstIndex=std::min(e.firstIndex,i);}
   if(v==second){++e.directSecond;e.secondIndex=std::min(e.secondIndex,i);}
   if(v<0x10000||(v&3u)||v>UINT32_MAX-8||v==first||v==second)continue;
   for(uint32_t off:{0u,4u}){
    uint32_t inner=0;
    if(!read(v+off,&inner,4))continue;
    if(inner==first){++e.indirectFirst;e.firstIndex=std::min(e.firstIndex,i);}
    if(inner==second){++e.indirectSecond;e.secondIndex=std::min(e.secondIndex,i);}
   }
  }
 }
 return e;
}
inline bool HasBoth(const CollectionEvidence& e){
 return (e.directFirst+e.indirectFirst)>0&&(e.directSecond+e.indirectSecond)>0;
}
inline bool IsSelectionLike(uint32_t rootOffset){
 return rootOffset==0x15c||rootOffset==0x160;
}
}
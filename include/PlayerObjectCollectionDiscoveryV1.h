#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstring>
namespace aoe2hd {
struct ObjectAnchorV1 {uint32_t address=0,id=0;};
struct CollectionEvidenceV1 {size_t matches=0,distinct=0;uint32_t mask=0;};
inline CollectionEvidenceV1 AnalyzeCandidateV1(const std::vector<uint8_t>& bytes,const std::vector<ObjectAnchorV1>& anchors){
 CollectionEvidenceV1 e;
 for(size_t i=0;i+4<=bytes.size();i+=4){
  uint32_t v=0;std::memcpy(&v,bytes.data()+i,4);
  for(size_t j=0;j<anchors.size()&&j<32;++j)
   if(v==anchors[j].address&&v!=0){++e.matches;e.mask|=(uint32_t(1)<<j);break;}
 }
 uint32_t bits=e.mask;while(bits){e.distinct+=bits&1u;bits>>=1;}return e;
}
}

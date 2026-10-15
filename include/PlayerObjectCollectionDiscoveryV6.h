#pragma once
#include "PlayerObjectCollectionDiscoveryV1.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <set>
#include <vector>
namespace aoe2hd {
struct TargetEvidenceV6 {uint32_t address=0,id=0;bool idReadable=false,exactAnchor=false;};
struct CandidateEvidenceV6 {
 size_t pointerSlots=0,uniqueTargets=0,readableIds=0,anchorMatches=0,distinctAnchors=0,plausibleIds=0;
 size_t longestRun=0,vectorTriples=0;uint32_t anchorMask=0;uint64_t fingerprint=1469598103934665603ull;
 double score=0;
};
template<class Validate>
CandidateEvidenceV6 InspectTargetV6(const std::vector<uint8_t>& bytes,
 const std::vector<ObjectAnchorV1>& anchors,Validate validate,size_t targetLimit=128){
 CandidateEvidenceV6 m;std::set<uint32_t> seen;size_t run=0;
 auto word=[&](size_t i){uint32_t v=0;std::memcpy(&v,bytes.data()+i,4);return v;};
 for(size_t i=0;i+4<=bytes.size();i+=4){
  uint32_t v=word(i);
  for(unsigned b=0;b<4;++b){m.fingerprint^=(v>>(b*8))&255u;m.fingerprint*=1099511628211ull;}
  bool ptr=v>=0x10000&&!(v&3u);
  if(ptr){
   ++m.pointerSlots;
   if(seen.insert(v).second&&seen.size()<=targetLimit){
    ++m.uniqueTargets;uint32_t id=0;
    if(validate(v,id)){
     ++m.readableIds;
     if(id>0&&id<10000000)++m.plausibleIds;
    }
   }
   ++run;m.longestRun=std::max(m.longestRun,run);
  }else run=0;
  for(size_t a=0;a<anchors.size()&&a<32;++a)if(v&&v==anchors[a].address){
   ++m.anchorMatches;m.anchorMask|=(uint32_t(1)<<a);
  }
  if(i+12<=bytes.size()){
   uint32_t end=word(i+4),cap=word(i+8);
   if(v>=0x10000&&end>=v&&cap>=end&&uint64_t(cap)-v<=131072 &&
      ((end-v)%4)==0&&((cap-v)%4)==0)++m.vectorTriples;
  }
 }
 for(uint32_t mask=m.anchorMask;mask;mask>>=1)m.distinctAnchors+=mask&1u;
 // Non-saturating diagnostics, not an entity confidence probability.
 m.score=m.distinctAnchors*100.0+m.readableIds*1.0+m.longestRun*0.25+
         m.vectorTriples*0.5;
 return m;
}
} // namespace aoe2hd

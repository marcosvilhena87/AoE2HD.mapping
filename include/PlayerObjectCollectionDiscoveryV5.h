#pragma once
#include "PlayerObjectCollectionDiscoveryV1.h"
#include <cstdint>
#include <cstring>
#include <vector>
#include <set>
#include <algorithm>
namespace aoe2hd {
struct CollectionMetricsV5 {
 size_t longestPointerRun=0,validPointerSlots=0,distinctAnchors=0,anchorMatches=0;
 size_t beginEndPairs=0,beginEndCapacityTriples=0,validatedObjectIDs=0;
 uint32_t anchorMask=0;
 double score=0;
};
template<class Probe,class ReadId>
CollectionMetricsV5 ExamineCollectionV5(const std::vector<uint8_t>& data,
 const std::vector<ObjectAnchorV1>& anchors,Probe readablePointer,ReadId objectId){
 CollectionMetricsV5 m;std::set<uint32_t> checked;
 size_t run=0;
 for(size_t i=0;i+4<=data.size();i+=4){
  uint32_t v=0;std::memcpy(&v,data.data()+i,4);
  bool valid=v>=0x10000&&!(v&3u)&&readablePointer(v);
  if(valid){++m.validPointerSlots;run++;m.longestPointerRun=std::max(m.longestPointerRun,run);}
  else run=0;
  for(size_t j=0;j<anchors.size()&&j<32;++j)if(v&&v==anchors[j].address){
   ++m.anchorMatches;m.anchorMask|=uint32_t(1)<<j;
  }
  if(valid&&checked.size()<64&&checked.insert(v).second){
   uint32_t id=0;
   if(objectId(v,id)&&id>0&&id<10000000)++m.validatedObjectIDs;
  }
  if(i+8<=data.size()){
   uint32_t end=0;std::memcpy(&end,data.data()+i+4,4);
   if(v>=0x10000&&end>=v&&end-v<=65536&&((end-v)%4)==0){
    ++m.beginEndPairs;
    if(i+12<=data.size()){
     uint32_t cap=0;std::memcpy(&cap,data.data()+i+8,4);
     if(cap>=end&&cap-v<=131072&&((cap-v)%4)==0)++m.beginEndCapacityTriples;
    }
   }
  }
 }
 for(uint32_t bits=m.anchorMask;bits;bits>>=1)m.distinctAnchors+=bits&1u;
 m.score=12.0*m.distinctAnchors+3.0*std::min<size_t>(m.longestPointerRun,32)
   +2.0*std::min<size_t>(m.validatedObjectIDs,16)
   +1.5*std::min<size_t>(m.beginEndCapacityTriples,12);
 return m;
}
}

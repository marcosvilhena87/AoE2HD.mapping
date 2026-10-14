#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>
namespace aoe2hd {
struct DualHit {uint32_t address=0,value=0;};
inline std::vector<DualHit> SearchTwoAnchors(uint32_t base,const uint8_t* bytes,
 size_t count,uint32_t a,uint32_t b,size_t limit){
 std::vector<DualHit> out;
 if(!bytes||!a||!b||a==b||!limit)return out;
 for(size_t i=0;i+4<=count&&out.size()<limit;++i){
  const uint64_t addr=uint64_t(base)+i;
  if(addr>UINT32_MAX-3)break;
  if(addr%4)continue;
  uint32_t value=0;std::memcpy(&value,bytes+i,4);
  if(value==a||value==b)out.push_back({uint32_t(addr),value});
 }
 return out;
}
struct PairCandidate {uint32_t first=0,second=0,distance=0;};
// Pairs within an inclusive bounded distance. Distinct addresses only.
inline std::vector<PairCandidate> FindNearbyPairs(const std::vector<DualHit>& input,
 uint32_t a,uint32_t b,uint32_t maxDistance,size_t limit){
 std::vector<DualHit> hits=input;
 std::sort(hits.begin(),hits.end(),[](const DualHit& x,const DualHit& y){return x.address<y.address;});
 std::vector<PairCandidate> out;
 for(size_t i=0;i<hits.size()&&out.size()<limit;++i){
  if(hits[i].value!=a)continue;
  for(size_t j=0;j<hits.size()&&out.size()<limit;++j){
   if(hits[j].value!=b)continue;
   const uint32_t distance=hits[i].address>hits[j].address?
    hits[i].address-hits[j].address:hits[j].address-hits[i].address;
   if(distance&&distance<=maxDistance)out.push_back({hits[i].address,hits[j].address,distance});
  }
 }
 return out;
}
}
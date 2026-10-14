#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
namespace aoe2hd {
struct PlayerCollectionHypothesis {
 uint32_t root=0,offset=0,begin=0,end=0,stride=0;
 uint32_t first=0,second=0,readable=0;
};
template<class Reader>
std::vector<PlayerCollectionHypothesis> DiscoverPlayerVectors(
 Reader&& read,uint32_t root,uint32_t first,uint32_t second,
 uint32_t rootBytes=0x800){
 std::vector<PlayerCollectionHypothesis> out;
 if(root<0x10000||rootBytes<12)return out;
 for(uint32_t off=0;off+12<=rootBytes;off+=4){
  uint32_t triple[3]{};
  if(!read(root+off,triple,sizeof(triple)))continue;
  auto b=triple[0],e=triple[1],c=triple[2];
  if(b<0x10000||(b&3)||e<b||c<e||e-b>65536||c-b>262144)continue;
  if(e==b)continue;
  for(uint32_t stride: {4u,8u,12u,16u}){
   if((e-b)%stride||((e-b)/stride)>4096)continue;
   PlayerCollectionHypothesis candidate{root,off,b,e,stride,0,0,0};
   for(uint32_t at=b;at<e;at+=stride){
    uint32_t ptr=0;
    if(!read(at,&ptr,4))continue;
    ++candidate.readable;
    candidate.first+=(ptr==first);
    candidate.second+=(ptr==second);
   }
   if(candidate.readable)out.push_back(candidate);
  }
 }
 return out;
}
}
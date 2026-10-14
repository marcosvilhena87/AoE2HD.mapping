#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
namespace aoe2hd {
struct CoverageRegion {uint32_t base=0,size=0;uint64_t attempted=0,read=0;uint32_t errors=0;};
inline bool IsStableReference(bool readable,uint32_t before,uint32_t after){
 return readable&&before==after;
}
inline bool IsSwitchingReference(bool readable,uint32_t before,uint32_t after,
 uint32_t first,uint32_t second){
 return readable&&before==second&&after==first;
}
inline bool BothAnchorsObserved(uint32_t firstCount,uint32_t secondCount){
 return firstCount>0&&secondCount>0;
}
template<class Reader>
std::vector<uint32_t> ReadPlayerRoots(Reader&& read,uint32_t world){
 std::vector<uint32_t> result;
 uint32_t begin=0,end=0;
 if(!read(world+0x184,&begin,4)||!read(world+0x188,&end,4)||
    begin<0x10000||end<begin||(end-begin)%8||(end-begin)/8>9)return result;
 for(uint32_t at=begin;at<end;at+=8){
  uint32_t player=0;if(read(at,&player,4)&&player>=0x10000&&!(player&3u))
   result.push_back(player);
 }
 return result;
}
}
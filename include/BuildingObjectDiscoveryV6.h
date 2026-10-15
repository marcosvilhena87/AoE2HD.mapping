#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstring>
#include <unordered_map>
namespace aoe2hd {
struct StrideScoreV6 {size_t stride=0,pairs=0,nonzeroPairs=0,equalNonzero=0,equalZero=0,commonMatches=0,rareMatches=0;double rate()const{return nonzeroPairs?double(equalNonzero)/nonzeroPairs:0.;}};
inline std::vector<StrideScoreV6> ScoreStridesV6(const std::vector<uint8_t>& buf,size_t minStride=128,size_t maxStride=2048,size_t step=4){
 std::vector<StrideScoreV6> result;
 if(step==0||step%4||minStride%4||maxStride%4||minStride>maxStride)return result;
 std::vector<uint32_t> words(buf.size()/4);
 std::unordered_map<uint32_t,size_t> frequency;
 for(size_t i=0;i<words.size();++i){std::memcpy(&words[i],buf.data()+i*4,4);if(words[i])++frequency[words[i]];}
 for(size_t stride=minStride;stride<=maxStride;stride+=step){
  StrideScoreV6 s;s.stride=stride;
  size_t gap=stride/4;
  for(size_t i=0;i+gap<words.size();++i){
   uint32_t a=words[i],b=words[i+gap];++s.pairs;
   if(a==0&&b==0){++s.equalZero;continue;}
   if(a&&b){++s.nonzeroPairs;if(a==b){++s.equalNonzero;if(frequency[a]>3)++s.commonMatches;else ++s.rareMatches;}}
  }
  result.push_back(s);
  if(maxStride-stride<step)break;
 }
 return result;
}
} // namespace aoe2hd

#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
namespace aoe2hd {
struct QueueWordV2 {uint16_t low=0,high=0;};
inline QueueWordV2 DecodeQueueWordV2(uint32_t v){return {uint16_t(v&65535u),uint16_t(v>>16)};}
inline bool QueueSequenceMatchesV2(const std::vector<uint16_t>& values,const std::vector<int>& counts){
 if(values.size()!=counts.size()||values.size()<3)return false;
 for(size_t i=1;i<values.size();++i)
  if(int(values[i])-int(values[i-1])!=counts[i]-counts[i-1])return false;
 return true;
}
inline uint32_t ReadWordV2(const std::vector<uint8_t>& v,size_t offset){
 uint32_t n=0;if(offset+4<=v.size())std::memcpy(&n,v.data()+offset,4);return n;
}
} // namespace aoe2hd

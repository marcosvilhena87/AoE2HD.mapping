#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
namespace aoe2hd {
struct WindowWordV4 {uint32_t offset=0,value=0;};
inline std::vector<WindowWordV4> DecodeWindowV4(const std::vector<uint8_t>& bytes){
 std::vector<WindowWordV4> v;
 for(size_t i=0;i+4<=bytes.size();i+=4){uint32_t x=0;std::memcpy(&x,bytes.data()+i,4);v.push_back({uint32_t(i),x});}
 return v;
}
inline bool IsCandidateObjectV4(uint32_t address,uint32_t expectedId,uint32_t actualId){
 return address>=0x10000&&!(address&3u)&&expectedId==actualId;
}
} // namespace aoe2hd

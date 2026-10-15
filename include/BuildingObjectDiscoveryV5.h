#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstring>
namespace aoe2hd {
struct StrideEvidenceV5 {size_t compared=0,nonzeroPairs=0,equalNonzero=0,equalZero=0;};
inline StrideEvidenceV5 CompareStrideV5(const std::vector<uint8_t>& buf,size_t stride){
 StrideEvidenceV5 r;
 if(stride==0||stride%4||stride>=buf.size())return r;
 for(size_t i=0;i+stride+4<=buf.size();i+=4){
  uint32_t a=0,b=0;std::memcpy(&a,buf.data()+i,4);std::memcpy(&b,buf.data()+i+stride,4);
  ++r.compared;
  if(a==0&&b==0){++r.equalZero;continue;}
  if(a!=0&&b!=0){++r.nonzeroPairs;if(a==b)++r.equalNonzero;}
 }
 return r;
}
} // namespace aoe2hd

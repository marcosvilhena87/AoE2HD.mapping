#pragma once
#include <cstdint>
namespace aoe2hd {
struct StageDiffV4 {bool changed[5]{};bool queueOnly=false,completionOnly=false;};
inline StageDiffV4 ClassifyStageDiffV4(const uint32_t (&v)[6]){
 StageDiffV4 result;
 for(int k=0;k<5;++k)result.changed[k]=v[k]!=v[k+1];
 const auto& d=result.changed;
 result.queueOnly=!d[0]&&d[1]&&!d[2]&&!d[3]&&!d[4];
 result.completionOnly=!d[0]&&!d[1]&&!d[2]&&d[3]&&!d[4];
 return result;
}

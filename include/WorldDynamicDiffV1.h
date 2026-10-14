#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
namespace aoe2hd {
struct DynamicDiff {uint32_t offset=0,before=0,control=0,after=0;bool controlChanged=false,creationChanged=false;};
inline std::vector<DynamicDiff> CompareThreeSnapshots(
 const std::vector<uint8_t>& before,const std::vector<uint8_t>& control,
 const std::vector<uint8_t>& after){
 std::vector<DynamicDiff> out;
 if(before.size()!=control.size()||before.size()!=after.size())return out;
 for(size_t i=0;i+4<=before.size();i+=4){
  DynamicDiff d;d.offset=uint32_t(i);
  std::memcpy(&d.before,before.data()+i,4);
  std::memcpy(&d.control,control.data()+i,4);
  std::memcpy(&d.after,after.data()+i,4);
  d.controlChanged=d.before!=d.control;
  d.creationChanged=d.control!=d.after;
  if(d.controlChanged||d.creationChanged)out.push_back(d);
 }
 return out;
}
}
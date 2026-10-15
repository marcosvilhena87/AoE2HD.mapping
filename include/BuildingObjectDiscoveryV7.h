#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
namespace aoe2hd {
struct ObjectIdentityV7 {uint32_t address=0,id=0;};
struct IdentityHitV7 {uint32_t slot=0,value=0;int matched=-1;};
inline std::vector<IdentityHitV7> MatchKnownObjectsV7(uint32_t base,const std::vector<uint8_t>& bytes,const std::vector<ObjectIdentityV7>& anchors){
 std::vector<IdentityHitV7> out;
 for(size_t i=0;i+4<=bytes.size();i+=4){
  uint32_t value=0;std::memcpy(&value,bytes.data()+i,4);
  for(size_t j=0;j<anchors.size();++j)
   if(value&&value==anchors[j].address){out.push_back({uint32_t(base+i),value,int(j)});break;}
 }
 return out;
}
}

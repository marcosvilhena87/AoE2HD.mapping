#pragma once
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <vector>
namespace aoe2hd {
struct DiffV7 {uint32_t offset=0,before=0,control=0,after=0;bool controlStable=false,incrementOne=false,newPointer=false;};
inline std::vector<DiffV7> CompareV7(const std::vector<uint8_t>& before,const std::vector<uint8_t>& control,const std::vector<uint8_t>& after){
 std::vector<DiffV7> out;
 if(before.size()!=control.size()||before.size()!=after.size()||before.size()%4)return out;
 for(size_t i=0;i<before.size();i+=4){
  uint32_t a=0,b=0,c=0;std::memcpy(&a,before.data()+i,4);
  std::memcpy(&b,control.data()+i,4);std::memcpy(&c,after.data()+i,4);
  if(a==b&&b==c)continue;
  DiffV7 d;d.offset=uint32_t(i);d.before=a;d.control=b;d.after=c;
  d.controlStable=a==b;d.incrementOne=d.controlStable&&a!=UINT32_MAX&&c==a+1;
  d.newPointer=d.controlStable&&a!=c&&c>=0x10000&&!(c&3u);
  out.push_back(d);
 }
 return out;
}
} // namespace aoe2hd

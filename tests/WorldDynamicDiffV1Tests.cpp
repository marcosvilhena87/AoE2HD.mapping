#include "WorldDynamicDiffV1.h"
#include <cassert>
#include <cstring>
int main(){
 std::vector<uint8_t> a(12),b(12),c(12);
 uint32_t x=10,y=11,z=12;
 std::memcpy(a.data(),&x,4);std::memcpy(b.data(),&y,4);std::memcpy(c.data(),&y,4);
 std::memcpy(c.data()+4,&z,4);
 auto diff=aoe2hd::CompareThreeSnapshots(a,b,c);
 assert(diff.size()==2);
 assert(diff[0].controlChanged&&!diff[0].creationChanged);
 assert(!diff[1].controlChanged&&diff[1].creationChanged);
 assert(aoe2hd::CompareThreeSnapshots(a,b,std::vector<uint8_t>(8)).empty());
}
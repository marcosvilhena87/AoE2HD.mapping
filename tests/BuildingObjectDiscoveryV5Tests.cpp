#include "BuildingObjectDiscoveryV5.h"
#include <cassert>
int main(){
 std::vector<uint8_t> b(24,0);
 b[0]=5;b[8]=5;b[4]=7;b[12]=9;
 auto e=aoe2hd::CompareStrideV5(b,8);
 assert(e.compared==4&&e.nonzeroPairs==2&&e.equalNonzero==1&&e.equalZero==0);
 assert(aoe2hd::CompareStrideV5(b,0).compared==0);
}

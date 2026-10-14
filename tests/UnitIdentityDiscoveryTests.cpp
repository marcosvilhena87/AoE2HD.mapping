#include "UnitIdentityDiscovery.h"
#include <cassert>
#include <cstring>
#include <vector>
int main(){
 std::vector<uint8_t> bytes(64,0);
 uint32_t a=2044,b=2045;
 std::memcpy(bytes.data()+8,&a,4);
 std::memcpy(bytes.data()+24,&b,4);
 auto x=aoe2hd::FindIdentityHits(bytes.data(),bytes.size(),a,b,16);
 assert(x.size()==2&&x[0].recordIndex==0&&x[0].fieldOffset==8);
 assert(x[1].recordIndex==1&&x[1].fieldOffset==8);
 assert(aoe2hd::BothIds(x,a,b));
 assert(!aoe2hd::BothIds(x,a,9999));
 assert(aoe2hd::FindIdentityHits(bytes.data(),bytes.size(),a,a,16).empty());
 assert(aoe2hd::FindIdentityHits(nullptr,0,a,b,16).empty());
 assert(aoe2hd::FindIdentityHits(bytes.data(),bytes.size(),a,b,16,1).size()==1);
}
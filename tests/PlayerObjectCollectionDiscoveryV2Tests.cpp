#include "PlayerObjectCollectionDiscoveryV2.h"
#include <cassert>
#include <cstring>
#include <map>
int main(){
 std::map<uint32_t,std::vector<uint8_t>> mem;
 for(auto p:{0x10000u,0x20000u,0x30000u})mem[p]=std::vector<uint8_t>(16,0);
 auto put=[&](uint32_t p,size_t off,uint32_t v){std::memcpy(mem[p].data()+off,&v,4);};
 put(0x10000,0,0x20000);put(0x20000,4,0x30000);put(0x30000,0,0x40000);
 auto reader=[&](uint32_t p,void* b,size_t n){auto it=mem.find(p);if(it==mem.end()||n!=16)return false;std::memcpy(b,it->second.data(),n);return true;};
 auto r=aoe2hd::TraverseObjectsV2({{0x10000,0,"World"}},{{0x40000,4}},reader,10,1024,3,16);
 assert(r.visited==3&&r.hits.size()==1&&r.hits[0].depth==2&&r.hits[0].target==0x40000);
 auto limited=aoe2hd::TraverseObjectsV2({{0x10000,0,"World"}},{{0x40000,4}},reader,2,1024,3,16);
 assert(limited.truncated&&limited.hits.empty());
}

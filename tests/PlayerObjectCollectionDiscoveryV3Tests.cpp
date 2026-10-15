#include "PlayerObjectCollectionDiscoveryV3.h"
#include <cassert>
#include <cstring>
#include <map>
int main(){
 std::map<uint32_t,std::vector<uint8_t>> mem;
 for(uint32_t x:{0x10000u,0x20000u,0x30000u,0x40000u,0x50000u,0x60000u})mem[x]=std::vector<uint8_t>(16,0);
 auto put=[&](uint32_t p,size_t off,uint32_t value){std::memcpy(mem[p].data()+off,&value,4);};
 put(0x10000,0,0x20000);put(0x20000,0,0x30000);put(0x30000,0,0x70000);
 put(0x40000,0,0x50000);put(0x50000,0,0x60000);put(0x60000,0,0x80000);
 auto read=[&](uint32_t p,void* b,size_t n){auto it=mem.find(p);if(it==mem.end()||n!=16)return false;std::memcpy(b,it->second.data(),n);return true;};
 aoe2hd::TraversalSettingsV3 settings;settings.scanBytes=16;settings.perDepthQuota={1,1,1,1};
 auto a=aoe2hd::TraverseObjectsV3({0x10000,0x40000},{{0x70000,1},{0x80000,2}},read,settings);
 assert(a.hits.size()==2&&a.hits[0].root==0&&a.hits[1].root==1);
 assert(a.hits[0].depth==2&&a.hits[1].depth==2);
 assert(a.buckets.size()==8&&a.buckets[2].successful==1&&a.buckets[6].successful==1);
 settings.perDepthQuota={1,1,0,0};
 auto b=aoe2hd::TraverseObjectsV3({0x10000,0x40000},{{0x70000,1}},read,settings);
 assert(b.truncated&&b.hits.empty()&&b.buckets[2].capDropped>0&&b.buckets[6].capDropped>0);
}

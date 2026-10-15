#include "PlayerObjectCollectionDiscoveryV4.h"
#include <cassert>
#include <cstring>
#include <map>
int main(){
 std::map<uint32_t,std::vector<uint8_t>> mem;
 for(uint32_t p:{0x10000u,0x20000u,0x30000u,0x40000u,0x50000u,0x60000u})
  mem[p]=std::vector<uint8_t>(16,0);
 auto put=[&](uint32_t p,size_t off,uint32_t v){std::memcpy(mem[p].data()+off,&v,4);};
 put(0x10000,0,0x20000);put(0x20000,0,0x30000);put(0x30000,0,0x70000);
 put(0x40000,0,0x50000);put(0x50000,0,0x60000);put(0x60000,0,0x80000);
 auto probe=[&](uint32_t p,size_t n){return n==16&&mem.count(p)!=0;};
 auto read=[&](uint32_t p,void* b,size_t n){if(!probe(p,n))return false;std::memcpy(b,mem[p].data(),n);return true;};
 aoe2hd::SettingsV4 cfg;cfg.scanBytes=16;cfg.quota={{1,1,1,1}};
 auto r=aoe2hd::TraverseV4({0x10000,0x40000},{{0x70000,1},{0x80000,2}},probe,read,cfg);
 assert(r.hits.size()==2&&r.hits[0].root==0&&r.hits[1].root==1);
 assert(r.hits[0].depth==2&&r.hits[1].depth==2);
 assert(r.buckets[2].readOK==1&&r.buckets[6].readOK==1);
 cfg.quota={{1,1,0,0}};
 auto limited=aoe2hd::TraverseV4({0x10000,0x40000},{{0x70000,1}},probe,read,cfg);
 assert(limited.truncated&&limited.hits.empty()&&limited.buckets[2].capDropped>0);
 auto bad=aoe2hd::TraverseV4({0x90000},{{0x70000,1}},probe,read,cfg);
 assert(bad.reads==0&&bad.buckets[0].preflightRejected==1);
}

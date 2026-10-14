#include "EntityGraphDiscovery.h"
#include <cassert>
#include <cstring>
#include <map>
int main(){
 std::map<uint32_t,uint32_t> mem{
  {0x10000,0x20000},{0x10004,0x20000},
  {0x20000,0x30000},{0x20004,0x40000},
  {0x40000,0x20000}};
 auto read=[&](uint32_t addr,void* dst,size_t n){
  auto it=mem.find(addr);if(n!=4||it==mem.end())return false;
  std::memcpy(dst,&it->second,4);return true;
 };
 auto g=aoe2hd::ExploreEntityGraph(read,{0x10000},0x30000,0x40000,16,100,8);
 assert(g.nodes==2);assert(!g.capped);
 unsigned a=0,b=0;
 for(auto& e:g.edges){a+=e.first;b+=e.second;assert(e.depth<=2);}
 assert(a==1&&b==1);
 auto limited=aoe2hd::ExploreEntityGraph(read,{0x10000},0x30000,0x40000,1,100,8);
 assert(limited.capped);
 assert(aoe2hd::ExploreEntityGraph(read,{0x10000},1,2,0,100,8).edges.empty());
}
#include "AdaptiveEntityGraph.h"
#include <cassert>
#include <cstring>
#include <map>
int main(){
 std::map<uint32_t,uint32_t> mem{{0x10000,0x30000},
  {0x20000+0x15c,0x90000},{0x20000+0x160,0x90000},
  {0x20000+0x200,0x40000},{0x30000,0x80000},{0x40000,0x90000}};
 auto read=[&](uint32_t a,void* p,size_t n){
  auto it=mem.find(a);if(n!=4||it==mem.end())return false;
  std::memcpy(p,&it->second,4);return true;
 };
 auto g=aoe2hd::ExploreAdaptiveGraph(read,0x10000,0x20000,0x80000,0x90000,20,100);
 unsigned selected=0,other=0;
 for(const auto& e:g.edges){
  if(e.second&&e.selection)++selected;
  if(e.first&&!e.selection)++other;
 }
 assert(selected==2&&other==1);
 assert(!g.capped);
 auto capped=aoe2hd::ExploreAdaptiveGraph(read,0x10000,0x20000,0x80000,0x90000,1,100);
 assert(capped.capped);
 return 0;
}
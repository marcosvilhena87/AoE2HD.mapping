#pragma once
#include <cstdint>
#include <queue>
#include <set>
#include <vector>
namespace aoe2hd {
struct AdaptiveEdge{
 uint32_t root=0,from=0,offset=0,to=0,depth=0;
 bool first=false,second=false,selection=false;
};
struct AdaptiveGraph{
 std::vector<AdaptiveEdge> edges;
 uint32_t nodes=0,readFailures=0;bool capped=false;
};
template<class Reader>
AdaptiveGraph ExploreAdaptiveGraph(Reader&& read,uint32_t world,uint32_t local,
 uint32_t first,uint32_t second,uint32_t maxNodes=512,uint32_t maxEdges=8192){
 AdaptiveGraph result;
 if(!world||!local||!maxNodes||!maxEdges)return result;
 struct Pending{uint32_t address,root,depth;};
 std::queue<Pending> queue;
 queue.push({world,world,0});queue.push({local,local,0});
 std::set<uint32_t> seen;
 while(!queue.empty()){
  auto cur=queue.front();queue.pop();
  if(seen.count(cur.address))continue;
  if(result.nodes>=maxNodes){result.capped=true;break;}
  seen.insert(cur.address);++result.nodes;
  // Known roots need +0x15C/+0x160. Unknown nodes use a smaller window.
  const uint32_t size=(cur.address==world||cur.address==local)?0x400u:0x100u;
  for(uint32_t off=0;off<size;off+=4){
   const uint64_t addr=uint64_t(cur.address)+off;
   if(addr>UINT32_MAX-3)break;
   uint32_t ptr=0;
   if(!read(uint32_t(addr),&ptr,4)){++result.readFailures;continue;}
   if(ptr<0x10000||(ptr&3u))continue;
   if(result.edges.size()>=maxEdges){result.capped=true;return result;}
   const bool selection=cur.address==local&&(off==0x15c||off==0x160);
   result.edges.push_back({cur.root,cur.address,off,ptr,cur.depth+1,
                           ptr==first,ptr==second,selection});
   // Two hops at most. Selection slots are evidence only, not expansion roots.
   if(cur.depth==0&&!selection&&!seen.count(ptr))
    queue.push({ptr,cur.root,1});
  }
 }
 return result;
}
}
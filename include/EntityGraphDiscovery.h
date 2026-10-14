#pragma once
#include <cstdint>
#include <queue>
#include <set>
#include <vector>
namespace aoe2hd {
struct GraphEdge {
 uint32_t root=0,from=0,offset=0,to=0,depth=0;
 bool first=false,second=false;
};
struct GraphResult{
 std::vector<GraphEdge> edges;
 uint32_t nodes=0,readFailures=0;
 bool capped=false;
};
// All addresses are hypotheses. Dedupe nodes; at most two pointer hops.
template<class Reader>
GraphResult ExploreEntityGraph(Reader&& read,const std::vector<uint32_t>& roots,
 uint32_t first,uint32_t second,uint32_t maxNodes=256,uint32_t maxEdges=4096,
 uint32_t bytesPerNode=256){
 GraphResult out;
 if(maxNodes==0||maxEdges==0||bytesPerNode<4||bytesPerNode>2048)return out;
 std::set<uint32_t> visited;
 struct Item{uint32_t address,root,depth;};
 std::queue<Item> pending;
 for(uint32_t root:roots)
  if(root>=0x10000 && !(root&3u))pending.push({root,root,0});
 while(!pending.empty()){
  auto item=pending.front();pending.pop();
  if(!visited.insert(item.address).second)continue;
  if(visited.size()>maxNodes){out.capped=true;break;}
  ++out.nodes;
  for(uint32_t offset=0;offset+4<=bytesPerNode;offset+=4){
   uint64_t field=uint64_t(item.address)+offset;
   if(field>UINT32_MAX-3)break;
   uint32_t ptr=0;
   if(!read(uint32_t(field),&ptr,4)){++out.readFailures;continue;}
   if(ptr<0x10000||(ptr&3u))continue;
   if(out.edges.size()>=maxEdges){out.capped=true;return out;}
   out.edges.push_back({item.root,item.address,offset,ptr,item.depth+1,
    ptr==first,ptr==second});
   if(item.depth<1&&visited.count(ptr)==0)pending.push({ptr,item.root,item.depth+1});
  }
 }
 return out;
}
}
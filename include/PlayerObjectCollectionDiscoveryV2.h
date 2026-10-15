#pragma once
#include "PlayerObjectCollectionDiscoveryV1.h"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <set>
#include <string>
#include <cstring>
namespace aoe2hd {
struct TraversalNodeV2 {uint32_t address=0;unsigned depth=0;std::string path;};
struct TraversalHitV2 {uint32_t slot=0,target=0;unsigned depth=0;std::string path;size_t anchorIndex=0;};
struct TraversalResultV2 {size_t visited=0,reads=0,bytes=0;bool truncated=false;std::vector<TraversalHitV2> hits;};
template<class Reader>
TraversalResultV2 TraverseObjectsV2(const std::vector<TraversalNodeV2>& roots,
 const std::vector<ObjectAnchorV1>& anchors,Reader read,
 size_t maxNodes=512,size_t maxBytes=2u*1024u*1024u,unsigned maxDepth=3,size_t scanBytes=1024){
 TraversalResultV2 out;
 std::vector<TraversalNodeV2> pending=roots;
 std::set<uint32_t> seen;
 for(size_t pos=0;pos<pending.size();++pos){
  if(out.visited>=maxNodes||out.bytes+scanBytes>maxBytes){out.truncated=true;break;}
  const auto node=pending[pos];
  if(node.depth>maxDepth||node.address<0x10000||(node.address&3u)||!seen.insert(node.address).second)continue;
  std::vector<uint8_t> bytes(scanBytes);
  ++out.reads;out.bytes+=scanBytes;
  if(!read(node.address,bytes.data(),bytes.size()))continue;
  ++out.visited;
  for(size_t off=0;off+4<=bytes.size();off+=4){
   uint32_t value=0;std::memcpy(&value,bytes.data()+off,4);
   for(size_t j=0;j<anchors.size();++j)
    if(value&&value==anchors[j].address){
     out.hits.push_back({uint32_t(node.address+off),value,node.depth,node.path+"+0x"+std::to_string(off),j});
    }
   if(node.depth<maxDepth&&value>=0x10000&&!(value&3u)&&pending.size()<maxNodes*16)
    pending.push_back({value,node.depth+1,node.path+"+0x"+std::to_string(off)});
  }
 }
 return out;
}
} // namespace aoe2hd

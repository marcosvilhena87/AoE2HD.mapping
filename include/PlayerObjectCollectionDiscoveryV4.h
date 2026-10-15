#pragma once
#include "PlayerObjectCollectionDiscoveryV1.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>
namespace aoe2hd {
inline std::string HexOffsetV4(size_t n){std::ostringstream s;s<<std::hex<<n;return s.str();}
struct CandidateV4 {uint32_t address=0;unsigned root=0,depth=0;std::string path;double priority=0;};
struct NodeEvidenceV4 {
 CandidateV4 node;std::string status;size_t rawPointers=0,validPointers=0,distinctPointers=0,anchorHits=0,childrenQueued=0;
 double pointerDensity=0;unsigned structuralHints=0;double score=0;
};
struct HitV4 {unsigned root=0,depth=0;uint32_t slot=0,target=0;size_t anchorIndex=0;std::string path;};
struct BucketV4 {size_t queued=0,attempted=0,readOK=0,readFailed=0,preflightRejected=0,duplicates=0,capDropped=0,hits=0;};
struct SettingsV4 {
 unsigned maxDepth=3;size_t scanBytes=1024,maxBytes=2u*1024u*1024u;
 size_t queueCap=512;std::array<size_t,4> quota={{1,48,96,96}};
};
struct ResultV4 {
 size_t reads=0,bytesAttempted=0,preflightCalls=0;bool truncated=false;
 std::vector<NodeEvidenceV4> nodes;std::vector<HitV4> hits;std::vector<BucketV4> buckets;
};
// probe(address,length) checks the exact address range against mapped readable committed memory.
// read(address,destination,length) performs a single bounded read. Both are injectable for tests.
template<class Probe,class Reader>
ResultV4 TraverseV4(const std::vector<uint32_t>& roots,const std::vector<ObjectAnchorV1>& anchors,
 Probe probe,Reader read,SettingsV4 cfg={}){
 ResultV4 out;
 if(roots.empty()||cfg.maxDepth>3||cfg.scanBytes<4||cfg.scanBytes%4||!cfg.queueCap)return out;
 const size_t levels=cfg.maxDepth+1;
 out.buckets.resize(roots.size()*levels);
 std::vector<std::vector<CandidateV4>> pending(roots.size()*levels);
 std::vector<std::set<uint32_t>> seen(roots.size());
 auto index=[&](size_t r,size_t d){return r*levels+d;};
 for(size_t r=0;r<roots.size();++r)pending[index(r,0)].push_back({roots[r],unsigned(r),0,r==0?"World":"LocalPlayer"});
 auto validRange=[&](uint32_t address)->bool{
  return address>=0x10000&&!(address&3u)&&uint64_t(address)+cfg.scanBytes<=0x100000000ull;
 };
 for(unsigned depth=0;depth<=cfg.maxDepth;++depth){
  for(size_t r=0;r<roots.size();++r){
   auto& queue=pending[index(r,depth)];auto& bucket=out.buckets[index(r,depth)];
   // Preserve diversity across immediate parent branches: sort by parent path, then round-robin.
   // Never rank solely by pointer count; high-density noise must not monopolize quota.
   std::vector<std::vector<CandidateV4>> groups;
   std::vector<std::string> keys;
   for(const auto& c:queue){
    auto slash=c.path.rfind("+0x");
    std::string key=slash==std::string::npos?c.path:c.path.substr(0,slash);
    auto it=std::find(keys.begin(),keys.end(),key);
    if(it==keys.end()){keys.push_back(key);groups.emplace_back();groups.back().push_back(c);}
    else groups[size_t(it-keys.begin())].push_back(c);
   }
   queue.clear();
   // Rank *within* each parent branch, while round-robin keeps branches diverse.
   for(auto& g:groups)std::stable_sort(g.begin(),g.end(),
    [](const CandidateV4& a,const CandidateV4& b){return a.priority>b.priority;});
   size_t remaining=0;for(const auto& g:groups)remaining+=g.size();
   for(size_t round=0;remaining;round++){
    bool any=false;
    for(auto& group:groups){
     if(round>=group.size())continue;
     any=true;--remaining;
     const auto node=group[round];
     if(!validRange(node.address)){++bucket.preflightRejected;out.nodes.push_back({node,"invalid"});continue;}
     if(!seen[r].insert(node.address).second){++bucket.duplicates;out.nodes.push_back({node,"duplicate"});continue;}
     if(bucket.attempted>=cfg.quota[depth]||out.bytesAttempted+cfg.scanBytes>cfg.maxBytes){
      ++bucket.capDropped;out.truncated=true;continue;
     }
     ++out.preflightCalls;
     if(!probe(node.address,cfg.scanBytes)){
      ++bucket.preflightRejected;out.nodes.push_back({node,"unreadable"});continue;
     }
     ++bucket.attempted;++out.reads;out.bytesAttempted+=cfg.scanBytes;
     std::vector<uint8_t> bytes(cfg.scanBytes);
     if(!read(node.address,bytes.data(),bytes.size())){
      ++bucket.readFailed;out.nodes.push_back({node,"read_failed"});continue;
     }
     ++bucket.readOK;
     NodeEvidenceV4 e;e.node=node;e.status="ok";
     std::vector<uint32_t> values;
     std::set<uint32_t> distinct;
     for(size_t off=0;off+4<=bytes.size();off+=4){
      uint32_t v=0;std::memcpy(&v,bytes.data()+off,4);
      for(size_t a=0;a<anchors.size();++a)if(v&&v==anchors[a].address){
       out.hits.push_back({unsigned(r),depth,uint32_t(node.address+off),v,a,node.path+"+0x"+HexOffsetV4(off)});
       ++e.anchorHits;++bucket.hits;
      }
      if(v>=0x10000&&!(v&3u)){++e.rawPointers;values.push_back(v);distinct.insert(v);}
     }
     e.distinctPointers=distinct.size();
     // Sample bounded set of unique targets: preflight is a signal, not proof of an object.
     size_t sampled=0;
     for(uint32_t candidate:distinct){
      if(sampled++>=64)break;
      ++out.preflightCalls;if(probe(candidate,cfg.scanBytes))++e.validPointers;
     }
     e.pointerDensity=double(e.validPointers)/double(bytes.size()/4);
     // Count plausible [begin,end] patterns, never label them verified collections.
     for(size_t off=0;off+8<=bytes.size();off+=4){
      uint32_t a=0,b=0;std::memcpy(&a,bytes.data()+off,4);std::memcpy(&b,bytes.data()+off+4,4);
      if(a>=0x10000&&b>=a&&uint64_t(b)-a<=65536&&((b-a)%4)==0)++e.structuralHints;
     }
     e.score=e.anchorHits*100.0+std::min<size_t>(e.validPointers,16)*2.0+
             std::min<unsigned>(e.structuralHints,8)*1.0;
     if(depth<cfg.maxDepth){
      auto& next=pending[index(r,depth+1)];
      // Parent diversity is preserved at processing time. Sample candidate offsets
      // across the full window rather than taking the first N only.
      const size_t limit=std::min<size_t>(values.size(),32);
      for(size_t j=0;j<limit;++j){
       size_t pos=(j*values.size())/limit;
       uint32_t target=values[pos];
       if(!validRange(target))continue;
       if(next.size()>=cfg.queueCap){out.truncated=true;++out.buckets[index(r,depth+1)].capDropped;continue;}
       next.push_back({target,unsigned(r),depth+1,node.path+"->0x"+HexOffsetV4(target),e.score});
       ++e.childrenQueued;++bucket.queued;
      }
     }
     out.nodes.push_back(e);
    }
    if(!any)break;
   }
  }
 }
 return out;
}
} // namespace aoe2hd

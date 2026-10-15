#pragma once
#include "PlayerObjectCollectionDiscoveryV1.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <sstream>
#include <iomanip>
#include <set>
#include <string>
#include <vector>
namespace aoe2hd {
inline std::string PointerOffsetHexV3(size_t offset){std::ostringstream out;out<<std::hex<<offset;return out.str();}
struct TraversalNodeV3 {uint32_t address=0;unsigned root=0,depth=0;std::string path;};
struct TraversalEventV3 {unsigned root=0,depth=0;uint32_t address=0;std::string path,status;size_t discovered=0,hits=0;};
struct TraversalHitV3 {unsigned root=0,depth=0;uint32_t slot=0,target=0;size_t anchor=0;std::string path;};
struct TraversalBucketV3 {size_t attempted=0,successful=0,readFailures=0,duplicates=0,invalid=0,queued=0,capDropped=0,hitCount=0;};
struct TraversalResultV3 {
 size_t bytesAttempted=0,reads=0;bool truncated=false;
 std::vector<TraversalEventV3> events;
 std::vector<TraversalHitV3> hits;
 std::vector<TraversalBucketV3> buckets;
};
struct TraversalSettingsV3 {
 unsigned maxDepth=3;
 size_t scanBytes=1024,maxBytes=2u*1024u*1024u,queueCapPerBucket=512;
 std::vector<size_t> perDepthQuota={1,48,96,96};
};
template<class Reader>
TraversalResultV3 TraverseObjectsV3(const std::vector<uint32_t>& roots,
 const std::vector<ObjectAnchorV1>& anchors,Reader read,
 TraversalSettingsV3 settings={}){
 TraversalResultV3 out;
 if(roots.empty()||settings.scanBytes<4||settings.scanBytes%4||
 settings.maxDepth>=settings.perDepthQuota.size()||!settings.queueCapPerBucket)return out;
 const size_t levels=settings.maxDepth+1;
 out.buckets.resize(roots.size()*levels);
 std::vector<std::deque<TraversalNodeV3>> pending(roots.size()*levels);
 auto at=[&](size_t r,size_t d){return r*levels+d;};
 std::vector<std::set<uint32_t>> seen(roots.size());
 for(size_t r=0;r<roots.size();++r)pending[at(r,0)].push_back({roots[r],unsigned(r),0,r==0?"World":"LocalPlayer"});
 for(unsigned d=0;d<=settings.maxDepth;++d){
  for(size_t r=0;r<roots.size();++r){
   auto index=at(r,d);auto& bucket=out.buckets[index];auto& queue=pending[index];
   while(!queue.empty()){
    if(bucket.attempted>=settings.perDepthQuota[d]||out.bytesAttempted+settings.scanBytes>settings.maxBytes){
     out.truncated=true;bucket.capDropped+=queue.size();queue.clear();break;
    }
    auto node=queue.front();queue.pop_front();
    if(node.address<0x10000||(node.address&3u)||uint64_t(node.address)+settings.scanBytes>0x100000000ull){
     ++bucket.invalid;out.events.push_back({unsigned(r),d,node.address,node.path,"invalid",0,0});continue;
    }
    if(!seen[r].insert(node.address).second){
     ++bucket.duplicates;out.events.push_back({unsigned(r),d,node.address,node.path,"duplicate",0,0});continue;
    }
    ++bucket.attempted;++out.reads;out.bytesAttempted+=settings.scanBytes;
    std::vector<uint8_t> bytes(settings.scanBytes);
    if(!read(node.address,bytes.data(),bytes.size())){
     ++bucket.readFailures;out.events.push_back({unsigned(r),d,node.address,node.path,"read_failed",0,0});continue;
    }
    ++bucket.successful;size_t discovered=0,hits=0;
    for(size_t off=0;off+4<=bytes.size();off+=4){
     uint32_t value=0;std::memcpy(&value,bytes.data()+off,4);
     for(size_t a=0;a<anchors.size();++a)if(value&&value==anchors[a].address){
      out.hits.push_back({unsigned(r),d,uint32_t(node.address+off),value,a,node.path+"+0x"+PointerOffsetHexV3(off)});
      ++hits;++bucket.hitCount;
     }
     if(d>=settings.maxDepth||value<0x10000||(value&3u))continue;
     auto& next=pending[at(r,d+1)];
     if(next.size()>=settings.queueCapPerBucket){++out.buckets[at(r,d+1)].capDropped;out.truncated=true;continue;}
     next.push_back({value,unsigned(r),d+1,node.path+"+0x"+PointerOffsetHexV3(off)});++discovered;++bucket.queued;
    }
    out.events.push_back({unsigned(r),d,node.address,node.path,"ok",discovered,hits});
   }
  }
 }
 return out;
}
} // namespace aoe2hd

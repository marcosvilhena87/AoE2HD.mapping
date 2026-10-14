#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>
namespace aoe2hd {
struct Collection {
 std::string source; uint32_t offset=0,begin=0,end=0,cap=0,stride=0;
 std::vector<uint32_t> values;
};
struct Snapshot {uint32_t pid=0,world=0,local=0; std::vector<Collection> collections;};
struct Change {
 std::string source,kind; uint32_t offset=0,stride=0,index=0,before=0,after=0;
};
inline std::vector<Change> Compare(const Snapshot& a,const Snapshot& b) {
 std::vector<Change> out;
 if(!a.pid||a.pid!=b.pid||!a.world||a.world!=b.world||a.local!=b.local)return out;
 using Key=std::tuple<std::string,uint32_t,uint32_t>;
 std::map<Key,const Collection*> left,right;
 for(const auto& c:a.collections)left[{c.source,c.offset,c.stride}]=&c;
 for(const auto& c:b.collections)right[{c.source,c.offset,c.stride}]=&c;
 std::map<Key,bool> keys;
 for(const auto& p:left)keys[p.first]=true;
 for(const auto& p:right)keys[p.first]=true;
 for(const auto& k:keys) {
  const auto ia=left.find(k.first),ib=right.find(k.first);
  const Collection* x=ia==left.end()?nullptr:ia->second;
  const Collection* y=ib==right.end()?nullptr:ib->second;
  if(!x||!y){out.push_back({std::get<0>(k.first),x?"collection_missing_after":"collection_new",std::get<1>(k.first), y?y->stride:x->stride,0,0,0});continue;}
  if(x->stride!=y->stride){out.push_back({std::get<0>(k.first),"stride_changed",std::get<1>(k.first),y->stride,0,x->stride,y->stride});continue;}
  if(x->begin!=y->begin)out.push_back({std::get<0>(k.first),"buffer_relocated",std::get<1>(k.first),y->stride,0,x->begin,y->begin});
  const size_t n=x->values.size()>y->values.size()?x->values.size():y->values.size();
  for(size_t i=0;i<n;++i) {
   if(i>=x->values.size()){out.push_back({std::get<0>(k.first),"added",std::get<1>(k.first),y->stride,static_cast<uint32_t>(i),0,y->values[i]});continue;}
   if(i>=y->values.size()){out.push_back({std::get<0>(k.first),"removed",std::get<1>(k.first),y->stride,static_cast<uint32_t>(i),x->values[i],0});continue;}
   if(x->values[i]!=y->values[i])out.push_back({std::get<0>(k.first),"changed",std::get<1>(k.first),y->stride,static_cast<uint32_t>(i),x->values[i],y->values[i]});
  }
 }
 return out;
}
} // namespace aoe2hd

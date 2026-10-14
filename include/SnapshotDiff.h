#pragma once
#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>
namespace aoe2hd {
struct Collection {
 std::string source; uint32_t offset=0,begin=0,end=0,cap=0,stride=0;
 // All entry words in index-major order: 1 word/entry for stride 4; 2 for stride 8.
 std::vector<uint32_t> values;
};
struct ObjectSample {
 uint32_t address=0;
 bool readable=false;
 std::vector<uint32_t> words; // 16 dwords = 64 bytes if readable
};
struct Snapshot {
 uint32_t pid=0,world=0,local=0;
 std::vector<Collection> collections;
 std::map<uint32_t,ObjectSample> objects;
};
struct Change {
 std::string source,kind;
 uint32_t offset=0,stride=0,index=0,word=0,before=0,after=0;
};
inline bool SameSession(const Snapshot& a,const Snapshot& b){
 return a.pid&&a.pid==b.pid&&a.world&&a.world==b.world&&a.local&&a.local==b.local;
}
inline std::vector<Change> Compare(const Snapshot& a,const Snapshot& b) {
 std::vector<Change> out;
 if(!SameSession(a,b))return out;
 using Key=std::tuple<std::string,uint32_t,uint32_t>;
 std::map<Key,const Collection*> left,right;
 for(const auto& c:a.collections)left[{c.source,c.offset,c.stride}]=&c;
 for(const auto& c:b.collections)right[{c.source,c.offset,c.stride}]=&c;
 std::map<Key,bool> keys;
 for(const auto& p:left)keys[p.first]=true;
 for(const auto& p:right)keys[p.first]=true;
 for(const auto& k:keys){
  auto ia=left.find(k.first),ib=right.find(k.first);
  const Collection* x=ia==left.end()?nullptr:ia->second;
  const Collection* y=ib==right.end()?nullptr:ib->second;
  const auto& source=std::get<0>(k.first);uint32_t off=std::get<1>(k.first),stride=std::get<2>(k.first);
  if(!x||!y){out.push_back({source,x?"collection_missing_after":"collection_new",off,stride});continue;}
  if(x->begin!=y->begin)out.push_back({source,"buffer_relocated",off,stride,0,0,x->begin,y->begin});
  const size_t wordsPerEntry=stride/4;
  if(wordsPerEntry==0)continue;
  const size_t n=std::max(x->values.size(),y->values.size());
  for(size_t i=0;i<n;++i){
   const bool xa=i<x->values.size(),yb=i<y->values.size();
   if(xa&&yb&&x->values[i]==y->values[i])continue;
   const char* kind=!xa?"added":!yb?"removed":"changed";
   out.push_back({source,kind,off,stride,static_cast<uint32_t>(i/wordsPerEntry),
    static_cast<uint32_t>(i%wordsPerEntry),xa?x->values[i]:0,yb?y->values[i]:0});
  }
 }
 // Compare memory content only at stable object addresses. Do not call a newly
 // discovered address an added object, because source collections are hypotheses.
 for(const auto& p:a.objects){
  auto it=b.objects.find(p.first);
  if(it==b.objects.end())continue;
  const auto& x=p.second;const auto& y=it->second;
  if(!x.readable||!y.readable)continue;
  const auto n=std::min(x.words.size(),y.words.size());
  for(size_t i=0;i<n;++i)if(x.words[i]!=y.words[i])
   out.push_back({"Object","object_field_changed",p.first,4,0,
     static_cast<uint32_t>(i),x.words[i],y.words[i]});
 }
 return out;
}
} // namespace aoe2hd

#include "PlayerUnitCollectionDiscovery.h"
#include <cassert>
#include <cstring>
#include <map>
int main(){
 std::map<uint32_t,uint32_t> memory{
  {0x10020,0x20000},{0x10024,0x20010},{0x10028,0x20020},
  {0x20000,0x30000},{0x20004,0},{0x20008,0x40000},{0x2000c,0}
 };
 auto read=[&](uint32_t a,void* out,size_t n){
  if(n==12){
   uint32_t tmp[3]{};
   for(int i=0;i<3;++i){auto it=memory.find(a+4*i);if(it==memory.end())return false;tmp[i]=it->second;}
   std::memcpy(out,tmp,n);return true;
  }
  auto it=memory.find(a);if(n!=4||it==memory.end())return false;
  std::memcpy(out,&it->second,4);return true;
 };
 auto c=aoe2hd::DiscoverPlayerVectors(read,0x10000,0x30000,0x40000,0x40);
 bool found=false;
 for(auto& x:c)if(x.offset==0x20&&x.stride==8&&x.first==1&&x.second==1)found=true;
 assert(found);
}
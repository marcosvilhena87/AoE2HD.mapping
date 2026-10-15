#include "ProductionQueueReader.h"
#include <cassert>
#include <cstring>
#include <vector>
int main(){
 using namespace aoe2hd;
 uint32_t ptr=0x20000,groups=3,units=4;
 std::vector<uint8_t> data={74,0,1,0,93,0,2,0,74,0,1,0};
 auto word=[&](uint32_t,uint32_t off,uint32_t& v){
  if(off==0x214)v=ptr;
  else if(off==0x218)v=groups<<16;
  else if(off==0x21c)v=units;
  else return false;
  return true;
 };
 auto bytes=[&](uint32_t p,void* dst,size_t n){
  if(p!=ptr||n>data.size())return false;
  std::memcpy(dst,data.data(),n);return true;
 };
 auto r=ReadProductionQueue(0x10000,word,bytes);
 assert(r.valid()&&r.queue.expanded==std::vector<uint16_t>({74,93,93,74}));
 groups=0;units=0;ptr=0;
 assert(ReadProductionQueue(0x10000,word,bytes).valid());
 groups=3;units=3;ptr=0x20000;
 assert(!ReadProductionQueue(0x10000,word,bytes).valid());
 units=4;ptr=0;
 assert(ReadProductionQueue(0x10000,word,bytes).status==QueueReadStatus::InvalidPointer);
 ptr=0x20000;
 auto changing=[&](uint32_t p,void* dst,size_t n){bool ok=bytes(p,dst,n);units=5;return ok;};
 assert(ReadProductionQueue(0x10000,word,changing).status==QueueReadStatus::ChangedDuringRead);
}

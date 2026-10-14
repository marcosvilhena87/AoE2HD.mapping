#include "PlayerIndirectTrackingV5.h"
#include <cassert>
#include <cstring>
int main(){
 auto read=[](uint32_t address,void* out,size_t n){
  if(address!=0x20000)return false;
  std::memset(out,7,n);return true;
 };
 auto a=aoe2hd::SnapshotContentV5(read,0x20000,32);
 assert(a.readable&&a.bytes.size()==32);
 auto b=a;b.bytes[3]=8;b.bytes[9]=9;
 assert(aoe2hd::ChangedBytesV5(a,b)==2);
 b.readable=false;assert(aoe2hd::ChangedBytesV5(a,b)==0);
 assert(!aoe2hd::SnapshotContentV5(read,0x30000,32).readable);
}
#include "PlayerOwnedStructuresV4.h"
#include <cassert>
#include <cstring>
int main(){
 assert(aoe2hd::UsefulOwnedPointer(0x20000,0x10000,0x30000,0x40000));
 assert(!aoe2hd::UsefulOwnedPointer(0x10000,0x10000,0x30000,0x40000));
 aoe2hd::OwnedField f{1,2,4,10,11};
 assert(aoe2hd::Changed(f));
 auto reader=[](uint32_t p,void* dst,size_t n){
  if(n!=4)return false;uint32_t v=p;std::memcpy(dst,&v,4);return true;
 };
 auto words=aoe2hd::ReadWords(reader,0x20000,16);
 assert(words.size()==4&&words[1]==0x20004);
}
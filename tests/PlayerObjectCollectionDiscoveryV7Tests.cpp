#include "PlayerObjectCollectionDiscoveryV7.h"
#include <cassert>
#include <cstring>
int main(){
 std::vector<uint8_t> a(12,0),b(12,0),c(12,0);
 auto put=[](std::vector<uint8_t>& x,size_t i,uint32_t v){std::memcpy(x.data()+i,&v,4);};
 put(a,0,5);put(b,0,5);put(c,0,6);
 put(a,4,12);put(b,4,13);put(c,4,14);
 put(a,8,0);put(b,8,0);put(c,8,0x20000);
 auto d=aoe2hd::CompareV7(a,b,c);
 assert(d.size()==3);
 assert(d[0].controlStable&&d[0].incrementOne&&!d[0].newPointer);
 assert(!d[1].controlStable&&!d[1].incrementOne);
 assert(d[2].controlStable&&d[2].newPointer);
 assert(aoe2hd::CompareV7(a,b,std::vector<uint8_t>(4)).empty());
}

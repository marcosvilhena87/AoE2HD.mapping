#include "PlayerObjectCollectionPathResolverV8.h"
#include <cassert>
int main(){
 aoe2hd::RootPathV8 p;
 assert(aoe2hd::ParseRootPathV8("World+0x88+0x14",p)&&p.relative&&p.world&&p.offsets.size()==2);
 uint32_t out=0;
 auto read=[](uint32_t slot,uint32_t& v){if(slot==0x10088)v=0x20000;else if(slot==0x20014)v=0x30000;else return false;return true;};
 assert(aoe2hd::ResolveRootPathV8(p,0x10000,0x40000,read,out)&&out==0x30000);
 assert(aoe2hd::ParseRootPathV8("LocalPlayer+0x15c",p)&&!p.world);
 assert(!aoe2hd::ParseRootPathV8("World+0x3",p));
 assert(!aoe2hd::ParseRootPathV8("World+0x1000",p));
 assert(!aoe2hd::ParseRootPathV8("World+0x88+0x14+0x20+0x24",p));
 assert(aoe2hd::ParseRootPathV8("0x14767530",p)&&!p.relative&&p.absolute==0x14767530);
 assert(aoe2hd::ResolveRootPathV8(p,0x10000,0x40000,read,out)&&out==0x14767530);
}

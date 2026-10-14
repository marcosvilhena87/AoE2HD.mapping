#include "WorldDynamicCounterV3.h"
#include <cassert>
int main(){
 auto a=aoe2hd::SplitCounter(0x00020000u);
 assert(a.low==0&&a.high==2);
 auto b=aoe2hd::SplitCounter(0x12345678u);
 assert(b.low==0x5678&&b.high==0x1234);
 assert(aoe2hd::Delta16(1,2)==1);
 assert(aoe2hd::Delta16(5,3)==-2);
}
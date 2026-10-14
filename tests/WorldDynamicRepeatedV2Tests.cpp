#include "WorldDynamicRepeatedV2.h"
#include <cassert>
int main(){
 uint32_t clean[5]={10,10,11,11,12};
 auto a=aoe2hd::AnalyzeRepeatedEvent(clean);
 assert(a.cleanRepeat&&a.incrementOne);
 uint32_t noisy[5]={10,11,12,12,13};
 auto b=aoe2hd::AnalyzeRepeatedEvent(noisy);
 assert(!b.cleanRepeat&&!b.incrementOne);
 uint32_t twice[5]={20,20,22,22,24};
 auto c=aoe2hd::AnalyzeRepeatedEvent(twice);
 assert(c.cleanRepeat&&!c.incrementOne);
 uint32_t wrap[5]={0xfffffffeu,0xfffffffeu,0xffffffffu,0xffffffffu,0u};
 auto d=aoe2hd::AnalyzeRepeatedEvent(wrap);
 assert(d.incrementOne);
}
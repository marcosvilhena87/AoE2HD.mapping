#include "WorldDynamicStagesV4.h"
#include <cassert>
int main(){
 const uint32_t queue[6]={10,10,11,11,11,11};
 auto q=aoe2hd::ClassifyStageDiffV4(queue);
 assert(q.queueOnly&&!q.completionOnly);
 const uint32_t done[6]={10,10,10,10,11,11};
 auto d=aoe2hd::ClassifyStageDiffV4(done);
 assert(d.completionOnly&&!d.queueOnly);
 const uint32_t noisy[6]={10,11,12,12,13,14};
 auto n=aoe2hd::ClassifyStageDiffV4(noisy);
 assert(!n.queueOnly&&!n.completionOnly);
}
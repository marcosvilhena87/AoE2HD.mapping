#include "ProductionQueueLayoutV2.h"
#include <cassert>
int main(){
 auto c=aoe2hd::DecodeQueueWordV2(65539u);
 assert(c.low==3&&c.high==1);
 assert(aoe2hd::QueueSequenceMatchesV2({0,1,2,3,2,1,0},{0,1,2,3,2,1,0}));
 assert(!aoe2hd::QueueSequenceMatchesV2({0,1,1},{0,1,2}));
}
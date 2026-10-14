#include "WorldDynamicQueueV5.h"
#include <cassert>
int main(){
 std::vector<aoe2hd::QueueObservationV5> v={
  {"initial",0,0,true,0,3},
  {"one",1,0,true,1,3},
  {"two",2,0,true,2,3},
  {"cancel",1,0,true,1,3},
  {"completed",0,1,true,0,4}
 };
 assert(aoe2hd::QueueMatchesV5(v));
 assert(aoe2hd::CompletionMatchesV5(v));
 v[3].queueField=2;
 assert(!aoe2hd::QueueMatchesV5(v));
 v[3].verified=false;
 assert(!aoe2hd::QueueMatchesV5(v));
}
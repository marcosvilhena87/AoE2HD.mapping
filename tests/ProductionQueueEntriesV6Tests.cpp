#include "ProductionQueueEntriesV6.h"
#include <cassert>
int main(){
 std::vector<uint8_t> buf={74,0,1,0,93,0,2,0,74,0,1,0};
 auto entries=aoe2hd::ReadQueueEntriesV6(buf);
 assert(entries.size()==3&&entries[0].type==74&&entries[1].quantity==2);
 assert(aoe2hd::CompareQueueV6(entries,{74,93,93,74},0,true));
 assert(!aoe2hd::CompareQueueV6(entries,{74,93,93,74},0,false));
 assert(!aoe2hd::CompareQueueV6(entries,{93,74},0,true));
 assert(aoe2hd::ReadQueueEntriesV6({74,0,1}).empty());
}

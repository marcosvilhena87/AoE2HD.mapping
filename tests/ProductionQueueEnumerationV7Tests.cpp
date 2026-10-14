#include "ProductionQueueEnumerationV7.h"
#include <cassert>
int main(){
 using aoe2hd::EnumerateQueueV7;
 std::vector<uint8_t> bytes={74,0,1,0,93,0,2,0,74,0,1,0,93,0,5,0};
 auto r=EnumerateQueueV7(bytes,3,4);
 assert(r.valid&&r.groups.size()==3&&r.expanded==std::vector<uint16_t>({74,93,93,74}));
 assert(EnumerateQueueV7({},0,0).valid);
 assert(!EnumerateQueueV7({},0,1).valid);
 assert(!EnumerateQueueV7(bytes,3,3).valid);
 assert(!EnumerateQueueV7(bytes,4,4).valid);
 assert(!EnumerateQueueV7(bytes,129,4).valid);
 std::vector<uint8_t> same={74,0,1,0,74,0,1,0};
 assert(!EnumerateQueueV7(same,2,2).valid);
}

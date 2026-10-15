#include "PlayerObjectCollectionDiscoveryV8.h"
#include <cassert>
int main(){
 aoe2hd::PointerSnapshotV8 a,b,c;
 a.slotRead=b.slotRead=c.slotRead=true;
 a.pointer=b.pointer=0x20000;c.pointer=0x30000;
 c.targetRead=true;c.id=2194;
 auto result=aoe2hd::ClassifyPointerV8(a,b,c,{2183,2193});
 assert(result.complete&&result.controlStable&&result.interventionChanged&&result.candidate);
 assert(!result.idEqualsKnown);
 b.pointer=0x28000;
 auto neg=aoe2hd::ClassifyPointerV8(a,b,c,{});
 assert(!neg.controlStable&&!neg.candidate&&neg.controlChanged);
 c.slotRead=false;
 assert(!aoe2hd::ClassifyPointerV8(a,b,c,{}).complete);
}

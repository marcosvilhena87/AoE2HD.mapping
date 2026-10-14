#include "SnapshotDiff.h"
#include <cassert>
#include <string>
#include <iostream>
int main(){
 using namespace aoe2hd;
 Snapshot a,b;a.pid=b.pid=1;a.world=b.world=0x10000;a.local=b.local=0x20000;
 Collection x;x.source="World";x.offset=0x88;x.stride=8;x.begin=0x30000;x.values={10,20,30};
 Collection y=x;y.begin=0x40000;y.values={10,25,30,40};
 a.collections.push_back(x);b.collections.push_back(y);
 auto d=Compare(a,b);assert(d.size()==3);
 assert(d[0].kind=="buffer_relocated" && d[1].kind=="changed" && d[2].kind=="added");
 b.world=0x99999;assert(Compare(a,b).empty());
 b.world=a.world;b.collections.clear();
 d=Compare(a,b);assert(d.size()==1&&d[0].kind=="collection_missing_after");
 b.collections.push_back(x);
 Collection z=x;z.stride=4;z.values={1};
 a.collections.push_back(z);
 d=Compare(a,b);assert(d.size()==1&&d[0].kind=="collection_missing_after");
 std::cout<<"SnapshotDiffTests OK\n";
}

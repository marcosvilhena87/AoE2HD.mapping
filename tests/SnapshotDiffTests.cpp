#include "SnapshotDiff.h"
#include <cassert>
#include <iostream>
#include <string>
int main(){
 using namespace aoe2hd;
 Snapshot a,b;a.pid=b.pid=1;a.world=b.world=0x10000;a.local=b.local=0x20000;
 Collection x;x.source="World";x.offset=0x88;x.stride=8;x.begin=0x30000;
 x.values={10,20,30,40};
 Collection y=x;y.values={10,25,30,40,50,60};y.begin=0x40000;
 a.collections.push_back(x);b.collections.push_back(y);
 auto d=Compare(a,b);
 assert(d.size()==4); // relocation, word1 changed, two added words
 assert(d[0].kind=="buffer_relocated");
 assert(d[1].kind=="changed" && d[1].index==0 && d[1].word==1 && d[1].before==20 && d[1].after==25);
 assert(d[2].kind=="added" && d[2].index==2 && d[2].word==0);
 assert(d[3].kind=="added" && d[3].index==2 && d[3].word==1);
 ObjectSample obj;obj.address=0x50000;obj.readable=true;obj.words={100,200,300};
 a.objects.emplace(obj.address,obj);obj.words[2]=301;b.objects.emplace(obj.address,obj);
 d=Compare(a,b);assert(d.size()==5);
 assert(d.back().kind=="object_field_changed" && d.back().offset==0x50000);
 assert(d.back().word==2 && d.back().before==300 && d.back().after==301);
 b.world=0x90000;assert(!SameSession(a,b));assert(Compare(a,b).empty());
 b.world=a.world;b.collections.clear();b.objects.clear();
 d=Compare(a,b);assert(d.size()==1&&d[0].kind=="collection_missing_after");
 b.collections.push_back(x);
 Collection alt=x;alt.stride=4;alt.values={10,20};a.collections.push_back(alt);
 d=Compare(a,b);assert(d.size()==1&&d[0].kind=="collection_missing_after");
 std::cout<<"SnapshotDiffTests OK\n";
}

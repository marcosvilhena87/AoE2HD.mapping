#include "UnitReferenceDiscovery.h"
#include "UnitCollectionDiscovery.h"
#include "UnitIdentityDiscovery.h"
#include "EntityGraphDiscovery.h"
#include "AdaptiveEntityGraph.h"
#include "StructuralGraphEvidence.h"
#include "AnchorPersistenceScan.h"
#include "DualAnchorDiscovery.h"
#include <map>
#include "ReferencePersistence.h"
#include "ProcessRegionScanner.h"
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
namespace {
struct Handle{HANDLE h;explicit Handle(HANDLE x):h(x){}~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}};
bool rd(HANDLE h,uint32_t addr,void* dest,size_t n){SIZE_T got=0;return ReadProcessMemory(h,reinterpret_cast<LPCVOID>(uintptr_t(addr)),dest,n,&got)&&got==n;}
bool u32(HANDLE h,uint32_t p,uint32_t off,uint32_t& out){return p<=UINT32_MAX-off&&rd(h,p+off,&out,4);}
uint32_t pid(){
 Handle h(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));
 if(h.h==INVALID_HANDLE_VALUE)return 0;
 PROCESSENTRY32W e{};e.dwSize=sizeof(e);
 if(!Process32FirstW(h.h,&e))return 0;
 do{if(_wcsicmp(e.szExeFile,L"AoK HD.exe")==0)return e.th32ProcessID;}while(Process32NextW(h.h,&e));
 return 0;
}
uint32_t module(uint32_t id){
 Handle h(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,id));
 if(h.h==INVALID_HANDLE_VALUE)return 0;
 MODULEENTRY32W e{};e.dwSize=sizeof(e);
 if(!Module32FirstW(h.h,&e))return 0;
 do{if(_wcsicmp(e.szModule,L"AoK HD.exe")==0){auto x=uintptr_t(e.modBaseAddr);return x<=UINT32_MAX?uint32_t(x):0;}}
 while(Module32NextW(h.h,&e));
 return 0;
}
struct Anchor{uint32_t owner=0,world=0,local=0,object=0,id=0,ownerCandidate=0,table=0;float hp=0;};
bool acquire(HANDLE h,uint32_t base,Anchor& a,std::string& reason){
 uint32_t owner=0,world=0,vt=0,idx=0,begin=0,end=0,cap=0;
 if(base>UINT32_MAX-0x006F2C58||!u32(h,base,0x006F2C58,owner)||!owner||
 !u32(h,owner,0xC8,world)||!world||!u32(h,world,0,vt)||
 vt!=base+0x005D329C||!u32(h,world,0x174,idx)||
 !u32(h,world,0x184,begin)||!u32(h,world,0x188,end)||
 !u32(h,world,0x18C,cap)||!begin||end<begin||cap<end||
 (end-begin)%8||(cap-begin)%8||(end-begin)/8<2||
 (cap-begin)/8>9||idx==0||idx>=(end-begin)/8){
 reason="world_not_ready";return false;
 }
 uint32_t local=0,control=0,pvt=0,back=0,slotA=0,slotB=0;
 if(!u32(h,begin,idx*8,local)||!u32(h,begin,idx*8+4,control)||
 !local||control>UINT32_MAX-12||local!=control+12||
 !u32(h,local,0,pvt)||pvt!=base+0x005DCAE4||
 !u32(h,local,4,back)||back!=world){
 reason="local_player_invalid";return false;
 }
 if(!u32(h,local,0x15c,slotA)||!u32(h,local,0x160,slotB)){
 reason="selection_unreadable";return false;
 }
 if(!slotA||slotA!=slotB){reason="select_exactly_one_unit";return false;}
 if(slotA<0x10000||(slotA&3u)||slotA>UINT32_MAX-0x38){
 reason="selection_invalid";return false;
 }
 uint32_t words[5]{},hpBits=0;
 if(!rd(h,slotA,words,sizeof(words))||!u32(h,slotA,0x34,hpBits)){
 reason="selected_object_unreadable";return false;
 }
 float hp;std::memcpy(&hp,&hpBits,4);
 if(!std::isfinite(hp)||hp<0||hp>100000){
 reason="selected_hp_invalid";return false;
 }
 uint32_t againA=0,againB=0,worldAgain=0,ownerAgain=0;
 if(!u32(h,local,0x15c,againA)||!u32(h,local,0x160,againB)||
 !u32(h,owner,0xC8,worldAgain)||!u32(h,base,0x006F2C58,ownerAgain)||
 againA!=slotA||againB!=slotB||worldAgain!=world||ownerAgain!=owner){
 reason="selection_changed";return false;
 }
 a={owner,world,local,slotA,words[2],words[4],words[3],hp};
 return true;
}
struct Region{std::string name;uint32_t offset=0,base=0,bytes=0,stride=0;};
std::vector<Region> regions(HANDLE h,const Anchor& a){
 std::vector<Region> result={{"World",0,a.world,0x800,4},{"LocalPlayer",0,a.local,0x800,4}};
 std::set<std::pair<uint32_t,uint32_t>> buffers;
 for(const auto& root:std::vector<std::pair<const char*,uint32_t>>{{"World",a.world},{"LocalPlayer",a.local}}){
  for(uint32_t off=0x10;off+12<=0x800;off+=4){
   uint32_t w[3]{};
   if(root.second>UINT32_MAX-off-12||!rd(h,root.second+off,w,12))continue;
   auto b=w[0],e=w[1],c=w[2];
   if(b<0x10000||b%4||e<=b||c<e||c-b>8192||e-b>2048)continue;
   if(!buffers.insert({b,e-b}).second)continue;
   result.push_back({std::string(root.first)+"Vector",off,b,e-b,4});
  }
 }
 return result;
}
}

struct ScanRegion {uint32_t base=0,size=0,protect=0;int priority=0;};
bool readable(const MEMORY_BASIC_INFORMATION& m){
 DWORD p=m.Protect&0xff;
 return m.State==MEM_COMMIT && !(m.Protect&PAGE_GUARD) &&
 (p==PAGE_READONLY||p==PAGE_READWRITE||p==PAGE_WRITECOPY||
 p==PAGE_EXECUTE_READ||p==PAGE_EXECUTE_READWRITE||p==PAGE_EXECUTE_WRITECOPY);
}
std::vector<ScanRegion> enumerate(HANDLE h,const Anchor& a){
 std::vector<ScanRegion> out;
 uintptr_t addr=0x10000;
 while(addr<0x7fff0000){
  MEMORY_BASIC_INFORMATION m{};
  if(!VirtualQueryEx(h,reinterpret_cast<LPCVOID>(addr),&m,sizeof(m)))break;
  uintptr_t base=reinterpret_cast<uintptr_t>(m.BaseAddress),end=base+m.RegionSize;
  if(end<=addr)break;
  if(readable(m)&&base<=UINT32_MAX&&m.RegionSize<=UINT32_MAX){
   bool key=(a.world>=base&&a.world<end)||(a.local>=base&&a.local<end)||
            (a.object>=base&&a.object<end);
   out.push_back({static_cast<uint32_t>(base),static_cast<uint32_t>(m.RegionSize),
                  m.Protect,key?0:1});
  }
  addr=end;
 }
 std::stable_sort(out.begin(),out.end(),[](const ScanRegion& x,const ScanRegion& y){
  if(x.priority!=y.priority)return x.priority<y.priority;
  return x.base<y.base;
 });
 return out;
}
struct Hit {uint32_t address=0,region=0,regionSize=0,protect=0,before=0,after=0;};


int main(int argc,char** argv){
 uint32_t maxMiB=128, perAnchor=2048;
 for(int i=1;i<argc;++i){
  const std::string opt=argv[i];
  if(i+1>=argc){std::cerr<<"Missing value\n";return 2;}
  char* end=nullptr;unsigned long v=std::strtoul(argv[++i],&end,10);
  if(!end||*end||v==0){std::cerr<<"Invalid value\n";return 2;}
  if(opt=="--max-mib"&&v<=512)maxMiB=uint32_t(v);
  else if(opt=="--per-anchor"&&v<=8192)perAnchor=uint32_t(v);
  else{std::cerr<<"Invalid option or range\n";return 2;}
 }
 const uint32_t id=pid(),base=id?module(id):0;
 if(!base){std::cerr<<"AoK HD.exe unavailable\n";return 2;}
 Handle h(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,id));
 if(!h.h){std::cerr<<"OpenProcess failed\n";return 2;}
 Anchor first,second;std::string why;
 if(!acquire(h.h,base,first,why)){std::cerr<<"First anchor: "<<why<<"\n";return 2;}
 std::cout<<"First anchor 0x"<<std::hex<<first.object<<std::dec<<" id="<<first.id
          <<"\nSelect a DIFFERENT villager and press ENTER.\n";
 std::string line;std::getline(std::cin,line);
 if(!acquire(h.h,base,second,why)||second.world!=first.world||
    second.local!=first.local||first.object==second.object||first.id==second.id){
  std::cerr<<"Second anchor invalid: "<<why<<"\n";return 3;
 }
 auto reader=[&](uint32_t p,void* dst,size_t n){return rd(h.h,p,dst,n);};
 uint32_t firstId=0;
 if(!u32(h.h,first.object,8,firstId)||firstId!=first.id){
  std::cerr<<"First object identity unstable\n";return 3;
 }
 // Scan every readable region in prioritized order, not merely regions
 // containing discovered graph nodes. Both anchors have separate hit limits.
 auto map=enumerate(h.h,second);
 std::vector<aoe2hd::AnchorReference> firstHits,secondHits;
 std::vector<uint8_t> chunk(65536);
 uint64_t examined=0;uint32_t attemptedRegions=0;
 const uint64_t budget=uint64_t(maxMiB)*1024*1024;
 for(const auto& region:map){
  if(examined>=budget||
     (firstHits.size()>=perAnchor&&secondHits.size()>=perAnchor))break;
  ++attemptedRegions;
  for(uint64_t off=0;off<region.size&&examined<budget;){
   if(firstHits.size()>=perAnchor&&secondHits.size()>=perAnchor)break;
   const size_t size=static_cast<size_t>(std::min<uint64_t>(
    {chunk.size(),uint64_t(region.size)-off,budget-examined}));
   if(size<4)break;
   uint32_t addr=region.base+static_cast<uint32_t>(off);
   SIZE_T got=0;
   BOOL ok=ReadProcessMemory(h.h,reinterpret_cast<LPCVOID>(uintptr_t(addr)),
                              chunk.data(),size,&got);
   examined+=size;
   if(ok&&got>=4)
    aoe2hd::CollectAnchorReferences(addr,region.base,chunk.data(),got,
      first.object,second.object,firstHits,secondHits,perAnchor);
   off+=size;
  }
 }
 Anchor stable;std::string r;
 if(!acquire(h.h,base,stable,r)||stable.object!=second.object||
    stable.id!=second.id||stable.world!=first.world||stable.local!=first.local||
    !u32(h.h,first.object,8,firstId)||firstId!=first.id){
  std::cerr<<"Anchors changed during scan; CSV not written\n";return 4;
 }
 std::cout<<"V4 first_hits="<<firstHits.size()<<" second_hits="<<secondHits.size()
          <<" bytes_examined="<<examined<<" regions="<<attemptedRegions<<"\n"
          <<"Now select the FIRST villager again, then press ENTER to test persistence.\n";
 std::getline(std::cin,line);
 Anchor third;std::string thirdWhy;
 if(!acquire(h.h,base,third,thirdWhy)||third.object!=first.object||
    third.id!=first.id||third.world!=first.world||third.local!=first.local||
    !u32(h.h,second.object,8,firstId)||firstId!=second.id){
  std::cerr<<"Third selection must be the FIRST villager; no CSV written: "
           <<thirdWhy<<"\n";return 5;
 }
 std::ofstream out("aoe2hd_entity_graph_reverse_v4.csv",std::ios::trunc);
 if(!out){std::cerr<<"CSV open failed\n";return 6;}
 out<<"pid,world,local,first_anchor,first_id,second_anchor,second_id,reference_address,region,initial_target,anchor_role,selection_slot,readable_after,observed_after,persistence,status\n";
 uint32_t persistentFirst=0,persistentSecond=0,switched=0;
 auto write=[&](const std::vector<aoe2hd::AnchorReference>& hits,const char* role){
  for(const auto& hit:hits){
   uint32_t observed=0;
   bool readable=rd(h.h,hit.address,&observed,4);
   const auto state=aoe2hd::ClassifyReference(readable,observed,first.object,second.object);
   persistentFirst+=state==aoe2hd::Persistence::first;
   persistentSecond+=state==aoe2hd::Persistence::second;
   switched+=(hit.value==second.object&&state==aoe2hd::Persistence::first);
   bool selection=hit.address==first.local+0x15c||hit.address==first.local+0x160;
   const char* category=state==aoe2hd::Persistence::first?"first":
     state==aoe2hd::Persistence::second?"second":
     state==aoe2hd::Persistence::other?"other":"unreadable";
   out<<id<<','<<first.world<<','<<first.local<<','<<first.object<<','<<first.id<<','
      <<second.object<<','<<second.id<<','<<hit.address<<','<<hit.region<<','
      <<hit.value<<','<<role<<','<<selection<<','<<readable<<',';
   if(readable)out<<observed;
   out<<','<<category<<",hypothesis\n";
  }
 };
 write(firstHits,"first");write(secondHits,"second");
 std::ofstream summary("aoe2hd_entity_graph_summary_v4.csv",std::ios::trunc);
 if(!summary){std::cerr<<"Summary open failed\n";return 6;}
 summary<<"pid,world,local,first_anchor,second_anchor,regions_visited,bytes_examined,first_hits,second_hits,first_limit,second_limit,first_persistent_after,second_persistent_after,second_to_first_switches,status\n"
        <<id<<','<<first.world<<','<<first.local<<','<<first.object<<','
        <<second.object<<','<<attemptedRegions<<','<<examined<<','
        <<firstHits.size()<<','<<secondHits.size()<<','
        <<(firstHits.size()>=perAnchor)<<','<<(secondHits.size()>=perAnchor)<<','
        <<persistentFirst<<','<<persistentSecond<<','<<switched<<",hypothesis\n";
 std::cout<<"V4 persistent_first="<<persistentFirst
          <<" persistent_second="<<persistentSecond
          <<" switched_second_to_first="<<switched<<"\n"
          <<"CSV: aoe2hd_entity_graph_reverse_v4.csv\n"
          <<"CSV: aoe2hd_entity_graph_summary_v4.csv\n"
          <<"Persistent pointers are hypotheses, not confirmed unit collections.\n";
 return 0;
}

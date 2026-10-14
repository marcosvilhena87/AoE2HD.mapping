#include "UnitReferenceDiscovery.h"
#include "UnitCollectionDiscovery.h"
#include "UnitIdentityDiscovery.h"
#include "EntityGraphDiscovery.h"
#include "AdaptiveEntityGraph.h"
#include "StructuralGraphEvidence.h"
#include "AnchorPersistenceScan.h"
#include "EntityGraphCoverageV5.h"
#include "UnitArrayCandidateValidator.h"
#include "PlayerUnitCollectionDiscovery.h"
#include "PlayerUnitCollectionInspectionV2.h"
#include "PlayerObjectClassificationV3.h"
#include "PlayerOwnedStructuresV4.h"
#include "PlayerIndirectTrackingV5.h"
#include "WorldDynamicDiffV1.h"
#include "WorldDynamicRepeatedV2.h"
#include "WorldDynamicCounterV3.h"
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


int main(){
 const uint32_t processId=pid(),base=processId?module(processId):0;
 if(!base){std::cerr<<"AoK HD.exe unavailable\n";return 2;}
 Handle process(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,processId));
 if(!process.h)return 2;
 Anchor a;std::string why,line;
 if(!acquire(process.h,base,a,why)){std::cerr<<"Select a known villager first: "<<why<<"\n";return 3;}
 auto read=[&](uint32_t p,void* dst,size_t n){return rd(process.h,p,dst,n);};
 const auto players=aoe2hd::ReadPlayerRoots(read,a.world);
 struct Target{uint32_t player;bool local;};
 std::vector<Target> targets;std::set<uint32_t> seen;
 for(uint32_t p:players)if(p>=0x10000&&seen.insert(p).second)
  targets.push_back({p,p==a.local});
 if(targets.empty()){std::cerr<<"No player roots\n";return 4;}
 std::cout<<"Select Town Center and keep selection unchanged. Press ENTER.\n";
 std::getline(std::cin,line);
 uint32_t selected=0,selected2=0;
 if(!u32(process.h,a.local,0x15c,selected)||!u32(process.h,a.local,0x160,selected2)||
    selected==0||selected!=selected2){std::cerr<<"Selection invalid\n";return 4;}
 struct Capture {std::string event;std::vector<std::vector<uint8_t>> bytes;bool stable=false;bool eventCompleted=false;};
 std::vector<Capture> captures;
 auto capture=[&](const std::string& event,bool completed){
  Capture c;c.event=event;c.eventCompleted=completed;
  uint32_t x=0,y=0;
  if(!u32(process.h,a.local,0x15c,x)||!u32(process.h,a.local,0x160,y)||
     x!=selected||y!=selected2)return false;
  for(const auto& t:targets){
   std::vector<uint8_t> buf(0x40);
   if(t.player>UINT32_MAX-0x3bc||!read(t.player+0x370,buf.data(),buf.size()))
    buf.clear();
   c.bytes.push_back(std::move(buf));
  }
  if(!u32(process.h,a.local,0x15c,x)||!u32(process.h,a.local,0x160,y)||
     x!=selected||y!=selected2)return false;
  c.stable=true;captures.push_back(std::move(c));return true;
 };
 if(!capture("baseline",false)){std::cerr<<"Initial capture failed\n";return 4;}
 const char* instructions[]={
  "CONTROL: no new units or research; press ENTER.",
  "VILLAGER_1: finish creating exactly one villager; press ENTER.",
  "VILLAGER_2: finish creating one more villager; press ENTER.",
  "NON_VILLAGER: finish creating exactly one non-villager unit if possible, otherwise press ENTER without action.",
  "RESEARCH: finish exactly one research if possible, otherwise press ENTER without action."
 };
 const char* names[]={"control","villager_1","villager_2","non_villager","research"};
 for(int i=0;i<5;++i){
  std::cout<<instructions[i]<<"\n";
  std::getline(std::cin,line);
  bool completed=(i==1||i==2);
  if(i>=3){
   std::cout<<"Was this optional event actually COMPLETED? Type y for yes, n for skipped: ";
   std::getline(std::cin,line);
   completed=(line=="y"||line=="Y");
  }
  if(!capture(names[i],completed)){std::cerr<<"Selection changed during "<<names[i]<<"; discard data\n";return 4;}
 }
 uint32_t ownerNow=0,worldNow=0,id=0;
 if(!u32(process.h,base,0x006F2C58,ownerNow)||ownerNow!=a.owner||
    !u32(process.h,a.owner,0xc8,worldNow)||worldNow!=a.world||
    !u32(process.h,a.object,8,id)||id!=a.id){std::cerr<<"Identity changed\n";return 4;}
 std::ofstream values("aoe2hd_world_dynamic_counter_values_v3.csv");
 std::ofstream changes("aoe2hd_world_dynamic_counter_deltas_v3.csv");
 if(!values||!changes)return 5;
 values<<"event,event_completed,player,role,offset,word,low16,high16,readable\n";
 changes<<"event,event_completed,player,role,offset,previous_low,current_low,delta_low,previous_high,current_high,delta_high,word_changed\n";
 for(size_t phase=0;phase<captures.size();++phase){
  const auto& c=captures[phase];
  for(size_t i=0;i<targets.size();++i){
   const auto& data=c.bytes[i];
   if(data.size()<0x10)continue;
   uint32_t word=aoe2hd::ReadWordEvent(data,0x0c);
   auto split=aoe2hd::SplitCounter(word);
   values<<c.event<<','<<c.eventCompleted<<','<<targets[i].player<<','
         <<(targets[i].local?"LocalPlayer":"Player")<<",892,"
         <<word<<','<<split.low<<','<<split.high<<",1\n";
   if(phase==0||captures[phase-1].bytes[i].size()<0x10)continue;
   auto previous=aoe2hd::SplitCounter(
      aoe2hd::ReadWordEvent(captures[phase-1].bytes[i],0x0c));
   changes<<c.event<<','<<c.eventCompleted<<','<<targets[i].player<<','
          <<(targets[i].local?"LocalPlayer":"Player")<<",892,"
          <<previous.low<<','<<split.low<<','<<aoe2hd::Delta16(previous.low,split.low)<<','
          <<previous.high<<','<<split.high<<','<<aoe2hd::Delta16(previous.high,split.high)<<','
          <<(word!=aoe2hd::ReadWordEvent(captures[phase-1].bytes[i],0x0c))<<"\n";
  }
 }
 std::ofstream adjacent("aoe2hd_world_dynamic_counter_neighborhood_v3.csv");
 if(!adjacent)return 5;
 adjacent<<"event,event_completed,player,offset,raw32\n";
 for(const auto& c:captures)for(size_t i=0;i<targets.size();++i){
  const auto& data=c.bytes[i];if(data.size()!=0x40)continue;
  for(uint32_t off=0;off<0x40;off+=4)
   adjacent<<c.event<<','<<c.eventCompleted<<','<<targets[i].player<<','<<(0x370+off)<<','
           <<aoe2hd::ReadWordEvent(data,off)<<"\n";
 }
 std::cout<<"WorldDynamicUnitDiscovery v3 phases="<<captures.size()
          <<" players="<<targets.size()<<"\nCSV: aoe2hd_world_dynamic_counter_values_v3.csv\n"
          <<"CSV: aoe2hd_world_dynamic_counter_deltas_v3.csv\n"
          <<"CSV: aoe2hd_world_dynamic_counter_neighborhood_v3.csv\n"
          <<"No counter semantics confirmed; skipped events must be reported separately.\n";
 return 0;
}

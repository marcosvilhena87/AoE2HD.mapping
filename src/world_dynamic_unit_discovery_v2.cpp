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
 if(!acquire(process.h,base,a,why)){std::cerr<<"Select one villager once to initialize: "<<why<<"\n";return 3;}
 auto read=[&](uint32_t p,void* dst,size_t n){return rd(process.h,p,dst,n);};
 auto players=aoe2hd::ReadPlayerRoots(read,a.world);
 struct Root{uint32_t address;std::string role;};
 std::vector<Root> roots{{a.world,"World"}};
 std::set<uint32_t> seen{a.world};
 for(uint32_t p:players)if(p>=0x10000&&seen.insert(p).second)
  roots.push_back({p,p==a.local?"LocalPlayer":"Player"});
 // The selection can change during setup but must remain unchanged
 // throughout all five captures: the selected Town Center is recommended.
 std::cout<<"Select the Town Center now and keep it selected throughout the experiment. Press ENTER.\n";
 std::getline(std::cin,line);
 uint32_t selection=0,selection2=0;
 auto selectionStable=[&](){
  uint32_t x=0,y=0;
  return u32(process.h,a.local,0x15c,x)&&u32(process.h,a.local,0x160,y)&&
         x==selection&&y==selection2;
 };
 if(!u32(process.h,a.local,0x15c,selection)||
    !u32(process.h,a.local,0x160,selection2)||
    !selection||selection!=selection2){std::cerr<<"Selection unavailable or inconsistent\n";return 4;}
 constexpr uint32_t length=0x800;
 struct Sample {Root root;std::vector<uint8_t> phases[5];};
 std::vector<Sample> samples;
 for(const auto& root:roots) samples.push_back({root,{}});
 auto capture=[&](int phase){
  if(!selectionStable())return false;
  for(auto& s:samples){
   auto& bytes=s.phases[phase];bytes.resize(length);
   if(s.root.address>UINT32_MAX-length||!read(s.root.address,bytes.data(),length))
    bytes.clear();
  }
  return selectionStable();
 };
 if(!capture(0)){std::cerr<<"Selection changed during initial capture\n";return 4;}
 const char* prompts[4]={
  "CONTROL 1: no new units; press ENTER.",
  "CREATE 1: produce exactly ONE villager; wait until it appears; press ENTER.",
  "CONTROL 2: no new units; press ENTER.",
  "CREATE 2: produce exactly ONE additional villager; wait until it appears; press ENTER."
 };
 for(int phase=1;phase<5;++phase){
  std::cout<<prompts[phase-1]<<"\n";
  std::getline(std::cin,line);
  if(!capture(phase)){std::cerr<<"Selection changed; discard experiment\n";return 4;}
 }
 uint32_t ownerNow=0,worldNow=0,id=0;
 if(!u32(process.h,base,0x006F2C58,ownerNow)||ownerNow!=a.owner||
    !u32(process.h,a.owner,0xc8,worldNow)||worldNow!=a.world||
    !u32(process.h,a.object,8,id)||id!=a.id){std::cerr<<"World/anchor changed\n";return 4;}
 std::ofstream fields("aoe2hd_world_dynamic_fields_v2.csv");
 std::ofstream coverage("aoe2hd_world_dynamic_roots_v2.csv");
 std::ofstream summary("aoe2hd_world_dynamic_summary_v2.csv");
 if(!fields||!coverage||!summary)return 5;
 fields<<"root,role,offset,initial,control1,creation1,control2,creation2,c1_changed,e1_changed,c2_changed,e2_changed,clean_repeat,increment_one\n";
 coverage<<"root,role,initial_ok,control1_ok,creation1_ok,control2_ok,creation2_ok,complete\n";
 uint32_t valid=0,changes=0,repeat=0,plusOne=0;
 for(const auto& s:samples){
  bool complete=true;
  coverage<<s.root.address<<','<<s.root.role;
  for(const auto& phase:s.phases){
   const bool ok=phase.size()==length;
   complete=complete&&ok;coverage<<','<<ok;
  }
  coverage<<','<<complete<<"\n";
  if(!complete)continue;
  ++valid;
  for(uint32_t offset=0;offset<length;offset+=4){
   uint32_t raw[5]{};
   for(int p=0;p<5;++p)raw[p]=aoe2hd::ReadWordEvent(s.phases[p],offset);
   auto f=aoe2hd::AnalyzeRepeatedEvent(raw);
   if(!(f.control1||f.creation1||f.control2||f.creation2))continue;
   ++changes;repeat+=f.cleanRepeat;plusOne+=f.incrementOne;
   fields<<s.root.address<<','<<s.root.role<<','<<offset;
   for(auto value:raw)fields<<','<<value;
   fields<<','<<f.control1<<','<<f.creation1<<','<<f.control2<<','
         <<f.creation2<<','<<f.cleanRepeat<<','<<f.incrementOne<<"\n";
  }
 }
 summary<<"pid,world,roots_seen,roots_complete,changed_fields,clean_repeat,increment_one,selection_stable\n"
        <<processId<<','<<a.world<<','<<samples.size()<<','<<valid<<','
        <<changes<<','<<repeat<<','<<plusOne<<",1\n";
 std::cout<<"WorldDynamicUnitDiscovery v2 complete_roots="<<valid
          <<" changed_fields="<<changes<<" clean_repeat="<<repeat
          <<" increment_one="<<plusOne<<"\n"
          <<"CSV: aoe2hd_world_dynamic_fields_v2.csv\n"
          <<"CSV: aoe2hd_world_dynamic_roots_v2.csv\n"
          <<"CSV: aoe2hd_world_dynamic_summary_v2.csv\n"
          <<"Correlation does not establish a unit count.\n";
 return 0;
}

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
 uint32_t processId=pid(),base=processId?module(processId):0;
 if(!base){std::cerr<<"AoK HD.exe unavailable\n";return 2;}
 Handle process(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,processId));
 if(!process.h){std::cerr<<"OpenProcess failed\n";return 2;}
 Anchor a;std::string why,line;
 if(!acquire(process.h,base,a,why)){std::cerr<<"Select one known villager: "<<why<<"\n";return 3;}
 auto read=[&](uint32_t p,void* dst,size_t n){return rd(process.h,p,dst,n);};
 auto players=aoe2hd::ReadPlayerRoots(read,a.world);
 struct Root{uint32_t address;std::string kind;};
 std::vector<Root> roots{{a.world,"World"}};
 std::set<uint32_t> seen{a.world};
 for(auto p:players)if(p>=0x10000&&seen.insert(p).second)
  roots.push_back({p,p==a.local?"LocalPlayer":"Player"});
 // This version deliberately snapshots fixed structural windows and does
 // not use the selected unit pointer to discover any collection.
 struct Sample{Root root;std::vector<uint8_t> initial,control,after;};
 std::vector<Sample> samples;
 const uint32_t length=0x800;
 auto capture=[&](uint32_t address,std::vector<uint8_t>& out){
  out.resize(length);if(address>UINT32_MAX-length||!read(address,out.data(),length)){
   out.clear();return false;
  }
  return true;
 };
 for(const auto& r:roots){
  Sample s;s.root=r;
  if(capture(r.address,s.initial))samples.push_back(std::move(s));
 }
 if(samples.empty()){std::cerr<<"No readable root windows\n";return 4;}
 std::cout<<"WorldDynamicUnitDiscovery v1 roots="<<samples.size()
          <<"\nCONTROL: let the game run without creating units; press ENTER.\n";
 std::getline(std::cin,line);
 for(auto& s:samples)capture(s.root.address,s.control);
 std::cout<<"CREATE exactly ONE villager, without selecting it. Press ENTER after creation.\n";
 std::getline(std::cin,line);
 uint32_t ownerNow=0,worldNow=0,idA=0;
 if(!u32(process.h,base,0x006F2C58,ownerNow)||ownerNow!=a.owner||
    !u32(process.h,a.owner,0xc8,worldNow)||worldNow!=a.world||
    !u32(process.h,a.object,8,idA)||idA!=a.id){
  std::cerr<<"World or anchor identity changed; comparison unsafe\n";return 4;
 }
 for(auto& s:samples)capture(s.root.address,s.after);
 std::ofstream out("aoe2hd_world_dynamic_fields_v1.csv",std::ios::trunc);
 std::ofstream coverage("aoe2hd_world_dynamic_roots_v1.csv",std::ios::trunc);
 std::ofstream summary("aoe2hd_world_dynamic_summary_v1.csv",std::ios::trunc);
 if(!out||!coverage||!summary){std::cerr<<"CSV open failed\n";return 5;}
 out<<"root,kind,offset,before,control,after,control_changed,creation_changed,creation_only,status\n";
 coverage<<"root,kind,initial_read,control_read,after_read,status\n";
 uint32_t creationOnly=0,controlChanged=0,creationChanged=0,validRoots=0;
 for(const auto& s:samples){
  const bool valid=s.initial.size()==length&&s.control.size()==length&&s.after.size()==length;
  coverage<<s.root.address<<','<<s.root.kind<<','<<!s.initial.empty()<<','
          <<!s.control.empty()<<','<<!s.after.empty()<<','
          <<(valid?"complete":"incomplete")<<"\n";
  if(!valid)continue;
  ++validRoots;
  auto changes=aoe2hd::CompareThreeSnapshots(s.initial,s.control,s.after);
  for(const auto& d:changes){
   bool only=!d.controlChanged&&d.creationChanged;
   creationOnly+=only;controlChanged+=d.controlChanged;
   creationChanged+=d.creationChanged;
   out<<s.root.address<<','<<s.root.kind<<','<<d.offset<<','<<d.before<<','
      <<d.control<<','<<d.after<<','<<d.controlChanged<<','
      <<d.creationChanged<<','<<only<<",hypothesis\n";
  }
 }
 summary<<"pid,world,roots_initial,roots_complete,control_changed,creation_changed,creation_only,status\n"
        <<processId<<','<<a.world<<','<<samples.size()<<','<<validRoots<<','
        <<controlChanged<<','<<creationChanged<<','<<creationOnly<<",hypothesis\n";
 std::cout<<"V1 complete_roots="<<validRoots<<" control_changed="<<controlChanged
          <<" creation_changed="<<creationChanged<<" creation_only="<<creationOnly<<"\n"
          <<"CSV: aoe2hd_world_dynamic_fields_v1.csv\n"
          <<"CSV: aoe2hd_world_dynamic_roots_v1.csv\n"
          <<"CSV: aoe2hd_world_dynamic_summary_v1.csv\n"
          <<"Changes are hypotheses, not evidence of unit ownership.\n";
 return 0;
}

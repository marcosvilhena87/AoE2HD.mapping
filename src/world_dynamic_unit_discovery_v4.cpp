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
#include "WorldDynamicStagesV4.h"
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
 if(!acquire(process.h,base,a,why)){std::cerr<<"Select one known villager initially: "<<why<<"\n";return 3;}
 auto read=[&](uint32_t p,void* d,size_t n){return rd(process.h,p,d,n);};
 struct Root{uint32_t address;std::string kind;};
 std::vector<Root> roots{{a.world,"World"}};std::set<uint32_t> seen{a.world};
 for(uint32_t player:aoe2hd::ReadPlayerRoots(read,a.world))
  if(player>=0x10000&&seen.insert(player).second)
   roots.push_back({player,player==a.local?"LocalPlayer":"Player"});
 std::cout<<"Select Town Center and leave it selected. Press ENTER.\n";
 std::getline(std::cin,line);
 uint32_t selection=0,other=0;
 if(!u32(process.h,a.local,0x15c,selection)||!u32(process.h,a.local,0x160,other)||
    !selection||selection!=other){std::cerr<<"Selection invalid\n";return 4;}
 struct Capture {std::string stage;bool operatorVerified=false;std::vector<std::vector<uint8_t>> roots;};
 std::vector<Capture> captures;
 auto take=[&](const std::string& stage,bool verified)->bool{
  uint32_t x=0,y=0;
  if(!u32(process.h,a.local,0x15c,x)||!u32(process.h,a.local,0x160,y)||
     x!=selection||y!=other)return false;
  Capture c;c.stage=stage;c.operatorVerified=verified;
  for(const auto& root:roots){
   std::vector<uint8_t> bytes(0x800);
   if(root.address>UINT32_MAX-0x800||!read(root.address,bytes.data(),bytes.size()))
    bytes.clear();
   c.roots.push_back(std::move(bytes));
  }
  if(!u32(process.h,a.local,0x15c,x)||!u32(process.h,a.local,0x160,y)||
     x!=selection||y!=other)return false;
  captures.push_back(std::move(c));return true;
 };
 if(!take("baseline",true)){std::cerr<<"Baseline unavailable\n";return 4;}
 struct Step{const char* name;const char* action;bool expected;};
 const Step steps[]={
  {"control","CONTROL: do nothing; press ENTER.",false},
  {"queued","QUEUE: start training exactly one villager, then press ENTER.",true},
  {"training","TRAINING: while villager is still training, press ENTER.",true},
  {"completed","COMPLETED: wait for villager to appear, then press ENTER.",true},
  {"post_control","POST CONTROL: no new production; press ENTER.",false}
 };
 for(const auto& step:steps){
  std::cout<<step.action<<"\n";
  std::getline(std::cin,line);
  bool verified=false;
  if(step.expected){
   std::cout<<"Did the described stage occur as requested? (y/n): ";
   std::getline(std::cin,line);verified=(line=="y"||line=="Y");
  }
  if(!take(step.name,verified)){std::cerr<<"Selection changed; snapshots discarded\n";return 4;}
 }
 uint32_t own=0,world=0,anchorId=0;
 if(!u32(process.h,base,0x006F2C58,own)||own!=a.owner||
    !u32(process.h,a.owner,0xc8,world)||world!=a.world||
    !u32(process.h,a.object,8,anchorId)||anchorId!=a.id){
  std::cerr<<"World/anchor invalidated\n";return 4;
 }
 std::ofstream fields("aoe2hd_world_dynamic_fields_v4.csv");
 std::ofstream rootsOut("aoe2hd_world_dynamic_roots_v4.csv");
 std::ofstream stages("aoe2hd_world_dynamic_stages_v4.csv");
 if(!fields||!rootsOut||!stages)return 5;
 fields<<"root,role,offset,baseline,control,queued,training,completed,post_control,changed_control,changed_queue,changed_training,changed_completion,changed_post_control,queue_only,completion_only\n";
 rootsOut<<"root,role,baseline_ok,control_ok,queued_ok,training_ok,completed_ok,post_control_ok\n";
 stages<<"stage,operator_confirmed,selection_stable,independent_event_verified\n";
 for(const auto& c:captures)
  stages<<c.stage<<','<<c.operatorVerified<<",1,0\n";
 uint32_t full=0,queueOnly=0,completeOnly=0,changed=0;
 for(size_t ri=0;ri<roots.size();++ri){
  bool valid=true;rootsOut<<roots[ri].address<<','<<roots[ri].kind;
  for(const auto& c:captures){
   bool ok=c.roots[ri].size()==0x800;valid&=ok;rootsOut<<','<<ok;
  }
  rootsOut<<"\n";if(!valid)continue;
  ++full;
  for(uint32_t offset=0;offset<0x800;offset+=4){
   uint32_t v[6]{};
   for(size_t k=0;k<captures.size();++k)
    v[k]=aoe2hd::ReadWordEvent(captures[k].roots[ri],offset);
   auto diff=aoe2hd::ClassifyStageDiffV4(v);
   const auto& d=diff.changed;
   if(!(d[0]||d[1]||d[2]||d[3]||d[4]))continue;
   ++changed;
   bool q=diff.queueOnly;
   bool c=diff.completionOnly;
   queueOnly+=q;completeOnly+=c;
   fields<<roots[ri].address<<','<<roots[ri].kind<<','<<offset;
   for(auto value:v)fields<<','<<value;
   for(bool value:d)fields<<','<<value;
   fields<<','<<q<<','<<c<<"\n";
  }
 }
 std::cout<<"WorldDynamicUnitDiscovery v4 roots_complete="<<full
          <<" changed_fields="<<changed<<" queue_only="<<queueOnly
          <<" completion_only="<<completeOnly<<"\n"
          <<"CSV: aoe2hd_world_dynamic_fields_v4.csv\n"
          <<"CSV: aoe2hd_world_dynamic_roots_v4.csv\n"
          <<"CSV: aoe2hd_world_dynamic_stages_v4.csv\n"
          <<"Operator confirmation is not independent verification of unit creation.\n";
 return 0;
}

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
#include "WorldDynamicQueueV5.h"
#include "ProductionQueueLayoutV2.h"
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
 Anchor tc;std::string reason,line;
 if(!acquire(process.h,base,tc,reason)){std::cerr<<"Select Town Center: "<<reason<<"\n";return 3;}
 std::cout<<"Object id="<<tc.id<<" address=0x"<<std::hex<<tc.object<<std::dec
          <<"\nConfirm Town Center with empty queue (y/n): ";
 std::getline(std::cin,line);if(line!="y"&&line!="Y")return 3;
 auto read=[&](uint32_t p,void* d,size_t n){return rd(process.h,p,d,n);};
 struct Target{uint32_t address,origin;};
 std::vector<Target> targets;
 std::vector<uint8_t> obj(0x1000);
 if(!read(tc.object,obj.data(),obj.size()))return 4;
 std::set<uint32_t> seen;
 for(uint32_t off=0;off<obj.size();off+=4){
  uint32_t ptr=aoe2hd::ReadWordV2(obj,off);
  if(ptr<0x10000||(ptr&3u)||ptr>UINT32_MAX-256||ptr==tc.object||
     ptr==tc.world||ptr==tc.local||!seen.insert(ptr).second)continue;
  std::vector<uint8_t> buf(256);
  if(read(ptr,buf.data(),buf.size())&&targets.size()<256)targets.push_back({ptr,off});
 }
 struct Phase {std::string name;int queue=-1;bool confirmed=false;std::vector<std::vector<uint8_t>> snapshots;};
 std::vector<Phase> phases;
 struct Step {const char* name;const char* instructions;};
 const Step steps[]={
  {"empty","EMPTY QUEUE: leave Town Center queue empty"},
  {"one","ADD 1: queue exactly one villager"},
  {"two","ADD 2: queue a second villager"},
  {"three","ADD 3: queue a third villager"},
  {"cancel","CANCEL 1: cancel one queued villager"},
  {"finish1","FINISH 1: wait for one villager to finish"},
  {"finish2","FINISH 2: wait for the second villager to finish"}
 };
 auto selected=[&](){
  uint32_t a=0,b=0,w=0,id=0;
  return u32(process.h,tc.local,0x15c,a)&&u32(process.h,tc.local,0x160,b)&&
   a==tc.object&&b==tc.object&&u32(process.h,tc.owner,0xc8,w)&&w==tc.world&&
   u32(process.h,tc.object,8,id)&&id==tc.id;
 };
 for(const auto& step:steps){
  std::cout<<step.instructions<<". Press ENTER once ready.\n";
  if(!std::getline(std::cin,line)||!selected()){std::cerr<<"Selection changed\n";return 4;}
  Phase phase;phase.name=step.name;
  std::cout<<"Queue count visually observed (include active training): ";
  if(!std::getline(std::cin,line))return 4;
  char* stop=nullptr;long count=std::strtol(line.c_str(),&stop,10);
  if(stop==line.c_str()||*stop!='\0'||count<0||count>1000)return 4;
  phase.queue=int(count);
  std::cout<<"Confirmed visually (y/n): ";
  if(!std::getline(std::cin,line))return 4;
  phase.confirmed=line=="y"||line=="Y";
  std::vector<uint8_t> root(0x1000);
  if(!read(tc.object,root.data(),root.size())){std::cerr<<"Town Center unreadable\n";return 4;}
  phase.snapshots.push_back(std::move(root));
  for(const auto& t:targets){
   std::vector<uint8_t> bytes(256);
   if(!read(t.address,bytes.data(),bytes.size()))bytes.clear();
   phase.snapshots.push_back(std::move(bytes));
  }
  if(!selected()){std::cerr<<"Selection changed during snapshot\n";return 4;}
  phases.push_back(std::move(phase));
 }
 std::ofstream fields("aoe2hd_production_queue_layout_fields_v2.csv");
 std::ofstream candidates("aoe2hd_production_queue_layout_candidates_v2.csv");
 std::ofstream events("aoe2hd_production_queue_layout_events_v2.csv");
 if(!fields||!candidates||!events)return 5;
 fields<<"region,origin_offset,field_offset,empty,one,two,three,cancel,finish1,finish2,matches_queue_deltas\n";
 candidates<<"region,origin_offset,offset,begin,end,capacity,entry_count,readable,interpretation\n";
 events<<"phase,observed_queue,operator_confirmed,independent_verified,queue_low16,queue_high16\n";
 std::vector<uint16_t> low;std::vector<int> counts;
 for(const auto& p:phases){
  auto word=aoe2hd::DecodeQueueWordV2(aoe2hd::ReadWordV2(p.snapshots[0],0x21c));
  low.push_back(word.low);counts.push_back(p.queue);
  events<<p.name<<','<<p.queue<<','<<p.confirmed<<",0,"<<word.low<<','<<word.high<<"\n";
 }
 bool allConfirmed=true;for(const auto& p:phases)allConfirmed&=p.confirmed;
 const bool counterConfirmed=allConfirmed&&aoe2hd::QueueSequenceMatchesV2(low,counts);
 uint32_t changed=0,matched=0,plausibleVectors=0;
 for(size_t region=0;region<=targets.size();++region){
  const uint32_t address=region?targets[region-1].address:tc.object;
  const uint32_t origin=region?targets[region-1].origin:0;
  const uint32_t len=region?256:0x1000;
  bool complete=true;
  for(const auto& p:phases)complete&=(p.snapshots[region].size()==len);
  if(!complete)continue;
  for(uint32_t off=0;off<len;off+=4){
   uint32_t v[7]{};
   for(size_t i=0;i<7;++i)v[i]=aoe2hd::ReadWordV2(phases[i].snapshots[region],off);
   bool different=false;for(int i=1;i<7;++i)different|=(v[i]!=v[0]);
   if(!different)continue;
   ++changed;
   bool match=allConfirmed;
   for(int i=1;i<7;++i)
    match&=int64_t(v[i])-int64_t(v[i-1])==counts[i]-counts[i-1];
   matched+=match;
   fields<<address<<','<<origin<<','<<off;
   for(auto x:v)fields<<','<<x;
   fields<<','<<match<<"\n";
  }
  // Search only bounded vector triples in each phase, preserving
  // historical root/field origins without treating candidates as proof.
  for(uint32_t off=0;off+12<=len;off+=4){
   uint32_t b=aoe2hd::ReadWordV2(phases[0].snapshots[region],off);
   uint32_t e=aoe2hd::ReadWordV2(phases[0].snapshots[region],off+4);
   uint32_t cap=aoe2hd::ReadWordV2(phases[0].snapshots[region],off+8);
   if(b<0x10000||(b&3u)||e<b||cap<e||cap-b>8192||e-b>4096)continue;
   if(e==b)continue;
   ++plausibleVectors;
   candidates<<address<<','<<origin<<','<<off<<','<<b<<','<<e<<','<<cap<<','
             <<(e-b)/4<<",1,vector_triple_hypothesis\n";
  }
 }
 std::cout<<"ProductionQueueStructureDiscovery v2 TownCenter+0x21C low16="
          <<low.back()<<" queue_counter_matches="<<counterConfirmed
          <<" changed_words="<<changed<<" matching_words="<<matched
          <<" vector_hypotheses="<<plausibleVectors<<"\n"
          <<"CSV: aoe2hd_production_queue_layout_fields_v2.csv\n"
          <<"CSV: aoe2hd_production_queue_layout_candidates_v2.csv\n"
          <<"CSV: aoe2hd_production_queue_layout_events_v2.csv\n"
          <<"No production entries or unit collection confirmed.\n";
 return 0;
}

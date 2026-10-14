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
 Anchor tc;std::string why,line;
 if(!acquire(process.h,base,tc,why)){std::cerr<<"Select exactly ONE Town Center first: "<<why<<"\n";return 3;}
 std::cout<<"Selected object ID="<<tc.id<<" address=0x"<<std::hex<<tc.object<<std::dec
          <<"\nConfirm selected object is a Town Center (y/n): ";
 std::getline(std::cin,line);if(line!="y"&&line!="Y")return 3;
 const uint32_t objectBytes=0x1000, targetBytes=0x80;
 auto read=[&](uint32_t p,void* d,size_t n){return rd(process.h,p,d,n);};
 struct Target{uint32_t address=0,field=0;};
 std::map<uint32_t,std::vector<uint32_t>> origins;
 std::vector<uint8_t> initialObject(objectBytes);
 if(!read(tc.object,initialObject.data(),objectBytes)){std::cerr<<"Town Center window unreadable\n";return 4;}
 // First-hop bounded target inventory. Discover once and retain address identities.
 for(uint32_t off=0;off<objectBytes;off+=4){
  uint32_t ptr=aoe2hd::ReadWordEvent(initialObject,off);
  if(ptr<0x10000||(ptr&3u)||ptr==tc.object||ptr==tc.world||
     ptr==tc.local||ptr>UINT32_MAX-targetBytes)continue;
  origins[ptr].push_back(off);
 }
 struct Region{uint32_t address;std::string role;uint32_t length;};
 std::vector<Region> regions{{tc.object,"TownCenter",objectBytes}};
 uint32_t attempted=0;
 for(const auto& entry:origins){
  if(regions.size()>=257)break;
  std::vector<uint8_t> tmp(targetBytes);
  ++attempted;
  if(read(entry.first,tmp.data(),targetBytes))regions.push_back({entry.first,"Indirect",targetBytes});
 }
 struct Phase{std::string name;int queue=0,completed=0;bool confirmed=false;std::vector<std::vector<uint8_t>> data;};
 std::vector<Phase> phases;
 struct Step{const char* name;const char* prompt;};
 const Step steps[]={
  {"baseline","BASELINE: empty Town Center production queue"},
  {"queue1","QUEUE 1: add one villager"},
  {"queue2","QUEUE 2: add a second villager"},
  {"queue3","QUEUE 3: add a third villager"},
  {"cancel","CANCEL: cancel one queued villager"},
  {"completion1","COMPLETION 1: wait for the first villager to appear"},
  {"completion2","COMPLETION 2: wait for the second villager to appear"}
 };
 auto count=[&](const char* message)->int{
  for(;;){
   std::cout<<message;
   if(!std::getline(std::cin,line))return -1;
   char* end=nullptr;long v=std::strtol(line.c_str(),&end,10);
   if(end!=line.c_str()&&*end=='\0'&&v>=0&&v<=1000)return int(v);
   std::cout<<"Use integer 0..1000\n";
  }
 };
 auto selected=[&](){
  uint32_t x=0,y=0,w=0,id=0;
  return u32(process.h,tc.local,0x15c,x)&&u32(process.h,tc.local,0x160,y)&&
   x==tc.object&&y==tc.object&&u32(process.h,tc.owner,0xc8,w)&&w==tc.world&&
   u32(process.h,tc.object,8,id)&&id==tc.id;
 };
 std::cout<<"Monitored regions="<<regions.size()<<" indirect_readable="<<regions.size()-1
          <<" candidates="<<origins.size()<<"\n";
 for(const auto& step:steps){
  std::cout<<step.prompt<<". Press ENTER when ready.\n";
  if(!std::getline(std::cin,line))return 4;
  Phase phase;phase.name=step.name;
  phase.queue=count("Observed units in production queue (include active): ");
  phase.completed=count("Villagers completed since baseline: ");
  if(phase.queue<0||phase.completed<0)return 4;
  std::cout<<"Were these observations visually verified? (y/n): ";
  std::getline(std::cin,line);phase.confirmed=(line=="y"||line=="Y");
  if(!selected()){std::cerr<<"Selection or Town Center identity changed; abort\n";return 4;}
  for(const auto& r:regions){
   std::vector<uint8_t> buffer(r.length);
   if(!read(r.address,buffer.data(),buffer.size()))buffer.clear();
   phase.data.push_back(std::move(buffer));
  }
  if(!selected()){std::cerr<<"Selection changed during capture; abort\n";return 4;}
  phases.push_back(std::move(phase));
 }
 std::ofstream fields("aoe2hd_production_queue_fields_v1.csv");
 std::ofstream paths("aoe2hd_production_queue_paths_v1.csv");
 std::ofstream phasesCsv("aoe2hd_production_queue_phases_v1.csv");
 if(!fields||!paths||!phasesCsv)return 5;
 fields<<"region,role,offset,baseline,queue1,queue2,queue3,cancel,completion1,completion2,delta_q1,delta_q2,delta_q3,delta_cancel,delta_completion1,delta_completion2,queue_delta_matches_all\n";
 paths<<"field_offset,target,monitored,readable_at_all_stages\n";
 phasesCsv<<"phase,queue_observed,completed_observed,operator_confirmed,independent_verified\n";
 for(const auto& p:phases)
  phasesCsv<<p.name<<','<<p.queue<<','<<p.completed<<','<<p.confirmed<<",0\n";
 uint32_t changed=0,queueMatched=0,completeRegions=0;
 for(size_t ri=0;ri<regions.size();++ri){
  bool complete=true;
  for(const auto& p:phases)complete&=(p.data[ri].size()==regions[ri].length);
  if(!complete)continue;
  ++completeRegions;
  for(uint32_t off=0;off<regions[ri].length;off+=4){
   uint32_t words[7]{};
   for(size_t k=0;k<phases.size();++k)words[k]=aoe2hd::ReadWordEvent(phases[k].data[ri],off);
   bool same=true;
   for(int k=1;k<7;++k)if(words[k]!=words[0])same=false;
   if(same)continue;
   ++changed;
   bool match=true;
   for(int k=1;k<7;++k){
    int64_t d=int64_t(words[k])-int64_t(words[k-1]);
    int expected=phases[k].queue-phases[k-1].queue;
    if(!phases[k].confirmed||!phases[k-1].confirmed||d!=expected)match=false;
   }
   queueMatched+=match;
   fields<<regions[ri].address<<','<<regions[ri].role<<','<<off;
   for(auto v:words)fields<<','<<v;
   for(int k=1;k<7;++k)fields<<','<<(int64_t(words[k])-int64_t(words[k-1]));
   fields<<','<<match<<"\n";
  }
 }
 std::set<uint32_t> monitored;for(const auto& r:regions)monitored.insert(r.address);
 for(const auto& entry:origins){
  bool tracked=monitored.count(entry.first)!=0;
  bool complete=false;
  if(tracked){
   auto it=std::find_if(regions.begin(),regions.end(),[&](const Region& r){return r.address==entry.first;});
   size_t i=size_t(it-regions.begin());complete=true;
   for(const auto& p:phases)complete&=(p.data[i].size()==targetBytes);
  }
  for(auto off:entry.second)paths<<off<<','<<entry.first<<','<<tracked<<','<<complete<<"\n";
 }
 std::cout<<"ProductionQueueStructureDiscovery v1 complete_regions="<<completeRegions
          <<" changed_words="<<changed<<" queue_matching_words="<<queueMatched<<"\n"
          <<"CSV: aoe2hd_production_queue_fields_v1.csv\n"
          <<"CSV: aoe2hd_production_queue_paths_v1.csv\n"
          <<"CSV: aoe2hd_production_queue_phases_v1.csv\n"
          <<"No unit list confirmed; completion is operator-reported.\n";
 return 0;
}

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
#include "ProductionQueueEntriesV6.h"
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
 if(!base){std::cerr<<"AoK HD.exe not running\n";return 2;}
 Handle process(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,processId));
 if(!process.h)return 2;
 Anchor b;std::string reason,line;
 if(!acquire(process.h,base,b,reason)){std::cerr<<"Select Barracks: "<<reason<<"\n";return 3;}
 std::cout<<"Object ID="<<b.id<<" address=0x"<<std::hex<<b.object<<std::dec
          <<"\nConfirm BARRACKS, empty queue (y/n): ";
 if(!std::getline(std::cin,line)||(line!="y"&&line!="Y"))return 3;
 auto selected=[&](){uint32_t a=0,c=0,w=0,id=0;return
   u32(process.h,b.local,0x15c,a)&&u32(process.h,b.local,0x160,c)&&
   a==b.object&&c==b.object&&u32(process.h,b.object,8,id)&&id==b.id&&
   u32(process.h,b.owner,0xc8,w)&&w==b.world;};
 struct Phase {std::string name,sequence;std::vector<uint16_t> expected;bool verified=false;
   uint32_t pointer=0,word218=0,word21c=0;bool readable=false;std::vector<uint8_t> buffer;};
 std::vector<Phase> phases;
 struct Step{const char* name;const char* prompt;};
 const Step steps[]={
 {"empty","EMPTY: no queue"},
 {"m","Queue Militia"},
 {"ms","Queue Spearman after Militia"},
 {"mss","Queue second consecutive Spearman"},
 {"mssm","Queue Militia after the two Spearmen"},
 {"cancel","Cancel last Militia if possible; report actual queue"},
 {"complete_m","Complete first Militia; report actual queue"},
 {"complete_s","Complete first Spearman; report actual queue"}};
 constexpr uint32_t bytes=512;
 auto parse=[](const std::string& str,std::vector<uint16_t>& out)->bool{
  if(str=="empty"||str=="EMPTY"||str=="-")return true;
  size_t start=0;
  while(start<str.size()){
   size_t stop=str.find('|',start);
   std::string token=str.substr(start,stop==std::string::npos?std::string::npos:stop-start);
   if(token=="M"||token=="Militia"||token=="militia")out.push_back(74);
   else if(token=="S"||token=="Spearman"||token=="spearman")out.push_back(93);
   else return false;
   if(stop==std::string::npos)break;
   start=stop+1;
  }
  return !out.empty();
 };
 for(const auto& step:steps){
  std::cout<<step.prompt<<". Press ENTER to capture.\n";
  if(!std::getline(std::cin,line)||!selected()){std::cerr<<"Selection changed\n";return 4;}
  Phase p;p.name=step.name;
  std::cout<<"Actual visible queue as M|S|S|M (M=Militia, S=Spearman, empty=empty): ";
  if(!std::getline(std::cin,p.sequence)||!parse(p.sequence,p.expected)){
   std::cerr<<"Invalid queue sequence\n";return 4;
  }
  std::cout<<"Queue visually verified (y/n): ";
  if(!std::getline(std::cin,line))return 4;
  p.verified=(line=="y"||line=="Y");
  if(!u32(process.h,b.object,0x214,p.pointer)||
     !u32(process.h,b.object,0x218,p.word218)||
     !u32(process.h,b.object,0x21c,p.word21c))return 4;
  if(p.pointer>=0x10000&&!(p.pointer&3u)&&p.pointer<=UINT32_MAX-bytes){
   p.buffer.resize(bytes);
   p.readable=rd(process.h,p.pointer,p.buffer.data(),bytes);
   if(!p.readable)p.buffer.clear();
  }
  if(!selected()){std::cerr<<"Selection changed while reading\n";return 4;}
  phases.push_back(std::move(p));
 }
 std::ofstream rows("aoe2hd_production_queue_entries_v6.csv");
 std::ofstream matches("aoe2hd_production_queue_alignment_v6.csv");
 std::ofstream meta("aoe2hd_production_queue_metadata_v6.csv");
 if(!rows||!matches||!meta)return 5;
 rows<<"phase,slot,offset,type_id,quantity,raw32,pointer\n";
 matches<<"phase,observed_queue,offset_slots,model,full_match,verified\n";
 meta<<"phase,observed_count,verified,pointer,readable,word218,word21c,low16_21c,high16_218,independently_verified\n";
 uint32_t perfect=0;
 for(const auto& p:phases){
  auto root=aoe2hd::DecodeQueueWordV2(p.word21c);
  auto mid=aoe2hd::DecodeQueueWordV2(p.word218);
  meta<<p.name<<','<<p.expected.size()<<','<<p.verified<<','<<p.pointer<<','
      <<p.readable<<','<<p.word218<<','<<p.word21c<<','<<root.low<<','<<mid.high<<",0\n";
  if(!p.readable)continue;
  auto entries=aoe2hd::ReadQueueEntriesV6(p.buffer,32);
  for(size_t k=0;k<entries.size();++k)
   rows<<p.name<<','<<k<<','<<4*k<<','<<entries[k].type<<','<<entries[k].quantity<<','
       <<aoe2hd::ReadWordV2(p.buffer,4*k)<<','<<p.pointer<<"\n";
  // Candidate active ranges are restricted to the observed count and are not
  // assumed valid simply because stale slot bytes resemble unit IDs.
  if(p.expected.empty())continue;
  for(size_t start=0;start<entries.size();++start){
   for(int aggregated=0;aggregated<=1;++aggregated){
    bool ok=aoe2hd::CompareQueueV6(entries,p.expected,start,aggregated!=0);
    if(ok&&p.verified)++perfect;
    matches<<p.name<<",\""<<p.sequence<<"\","<<start<<','
           <<(aggregated?"run_grouped":"per_order")<<','<<(ok&&p.verified)<<','<<p.verified<<"\n";
   }
  }
 }
 std::cout<<"ProductionQueueStructureDiscovery v6 stages="<<phases.size()
          <<" verified_alignment_candidates="<<perfect<<"\n"
          <<"CSV: aoe2hd_production_queue_entries_v6.csv\n"
          <<"CSV: aoe2hd_production_queue_alignment_v6.csv\n"
          <<"CSV: aoe2hd_production_queue_metadata_v6.csv\n"
          <<"Candidate matches are NOT validation of active bounds or a living-unit list.\n";
 return 0;
}

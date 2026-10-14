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
 Anchor b;std::string reason,line;
 if(!acquire(process.h,base,b,reason)){std::cerr<<"Select exactly one production building: "<<reason<<"\n";return 3;}
 std::cout<<"Selected building object ID="<<b.id<<" address=0x"<<std::hex<<b.object<<std::dec<<"\n";
 std::cout<<"Building kind (TownCenter/Barracks/Stable/Other): ";
 if(!std::getline(std::cin,line))return 3;const std::string building=line;
 std::cout<<"Confirm selected object is this production building, queue empty (y/n): ";
 if(!std::getline(std::cin,line)||(line!="y"&&line!="Y"))return 3;
 auto stable=[&](){uint32_t x=0,y=0,id=0,w=0;return
 u32(process.h,b.local,0x15c,x)&&u32(process.h,b.local,0x160,y)&&x==b.object&&y==b.object&&
 u32(process.h,b.object,8,id)&&id==b.id&&u32(process.h,b.owner,0xc8,w)&&w==b.world;};
 struct Sample{std::string label,unit;int observed=0;bool verified=false;
 uint32_t ptr=0,word218=0,word21c=0;bool readable=false;std::vector<uint8_t> data;};
 std::vector<Sample> samples;
 const char* steps[]={"empty","one","two","three","cancel","finish1","finish2"};
 const char* guides[]={"EMPTY: zero orders","ONE: enqueue one unit","TWO: enqueue second unit of SAME type",
 "THREE: enqueue third unit of SAME type","CANCEL: cancel one queued unit",
 "FINISH1: complete one unit","FINISH2: complete another unit"};
 for(int i=0;i<7;++i){
  std::cout<<guides[i]<<". Press ENTER for capture.\n";
  if(!std::getline(std::cin,line)||!stable()){std::cerr<<"Selection changed\n";return 4;}
  Sample s;s.label=steps[i];
  std::cout<<"Unit type queued (e.g. Villager/Militia/Scout; same label all stages): ";
  if(!std::getline(std::cin,s.unit))return 4;
  std::cout<<"Observed queue size (including training): ";
  if(!std::getline(std::cin,line))return 4;
  char* end=nullptr;long n=std::strtol(line.c_str(),&end,10);
  if(end==line.c_str()||*end!='\0'||n<0||n>1000)return 4;s.observed=int(n);
  std::cout<<"Queue and unit label visually verified (y/n): ";
  if(!std::getline(std::cin,line))return 4;s.verified=(line=="y"||line=="Y");
  if(!u32(process.h,b.object,0x214,s.ptr)||!u32(process.h,b.object,0x218,s.word218)||
     !u32(process.h,b.object,0x21c,s.word21c))return 4;
  if(s.ptr>=0x10000&&!(s.ptr&3u)&&s.ptr<=UINT32_MAX-512){
   s.data.resize(512);s.readable=rd(process.h,s.ptr,s.data.data(),s.data.size());
   if(!s.readable)s.data.clear();
  }
  if(!stable()){std::cerr<<"Selection changed during capture\n";return 4;}
  samples.push_back(std::move(s));
 }
 std::ofstream out("aoe2hd_production_queue_types_v4.csv");
 std::ofstream changes("aoe2hd_production_queue_type_changes_v4.csv");
 if(!out||!changes)return 5;
 out<<"building,building_object,stage,operator_unit_label,observed_queue,operator_verified,ptr_214,word_218,word_21c,queue_low16,queue_high16,ptr_readable,entry_word0,entry_type_low16,entry_quantity_high16,independent_verified\n";
 changes<<"from,to,same_ptr,readable_both,offset,before,after\n";
 uint32_t readable=0,changed=0;bool match=true;bool complete=true;
 for(size_t i=0;i<samples.size();++i){
  const auto& s=samples[i];auto counter=aoe2hd::DecodeQueueWordV2(s.word21c);
  uint32_t entry=s.readable?aoe2hd::ReadWordV2(s.data,0):0;
  auto decoded=aoe2hd::DecodeQueueWordV2(entry);readable+=s.readable;complete&=s.verified;
  out<<building<<','<<b.object<<','<<s.label<<','<<s.unit<<','<<s.observed<<','<<s.verified<<','
   <<s.ptr<<','<<s.word218<<','<<s.word21c<<','<<counter.low<<','<<counter.high<<','
   <<s.readable<<','<<entry<<','<<decoded.low<<','<<decoded.high<<",0\n";
  if(i){
   const auto& a=samples[i-1];
   if(!a.verified||!s.verified||int(counter.low)-int(aoe2hd::DecodeQueueWordV2(a.word21c).low)!=s.observed-a.observed)match=false;
   bool comparable=a.readable&&s.readable&&a.ptr==s.ptr;
   if(comparable)for(uint32_t off=0;off<512;off+=4){
    auto prev=aoe2hd::ReadWordV2(a.data,off),next=aoe2hd::ReadWordV2(s.data,off);
    if(prev!=next){++changed;changes<<a.label<<','<<s.label<<",1,1,"<<off<<','<<prev<<','<<next<<"\n";}
   }
   else changes<<a.label<<','<<s.label<<','<<(a.ptr==s.ptr)<<",0,-1,0,0\n";
  }
 }
 std::cout<<"ProductionQueueStructureDiscovery v4 building="<<building
          <<" readable_stages="<<readable<<" changed_words="<<changed
          <<" queue_counter_matches="<<(match&&complete)<<"\n"
          <<"CSV: aoe2hd_production_queue_types_v4.csv\n"
          <<"CSV: aoe2hd_production_queue_type_changes_v4.csv\n"
          <<"Compare separately executed buildings; no shared layout assumed.\n";
 return 0;
}

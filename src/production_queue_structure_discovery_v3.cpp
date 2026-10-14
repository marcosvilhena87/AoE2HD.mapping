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
 std::cout<<"Object ID="<<tc.id<<" address=0x"<<std::hex<<tc.object<<std::dec
          <<"\nConfirm Town Center with an empty queue (y/n): ";
 if(!std::getline(std::cin,line)||(line!="y"&&line!="Y"))return 3;
 struct Snapshot{std::string stage;int observed=0;bool verified=false;uint32_t pointer=0,word218=0,word21c=0;bool readable=false;std::vector<uint8_t> bytes;};
 std::vector<Snapshot> snapshots;
 const char* names[]={"empty","queue1","queue2","queue3","cancel","finish1","finish2"};
 const char* actions[]={"EMPTY: queue is empty","QUEUE1: add first villager","QUEUE2: add second villager","QUEUE3: add third villager","CANCEL: cancel one","FINISH1: finish one villager","FINISH2: finish another villager"};
 auto stable=[&](){
  uint32_t x=0,y=0,w=0,id=0;
  return u32(process.h,tc.local,0x15c,x)&&u32(process.h,tc.local,0x160,y)&&
   x==tc.object&&y==tc.object&&u32(process.h,tc.owner,0xc8,w)&&w==tc.world&&
   u32(process.h,tc.object,8,id)&&id==tc.id;
 };
 constexpr uint32_t window=512;
 for(int i=0;i<7;++i){
  std::cout<<actions[i]<<". Press ENTER to capture.\n";
  if(!std::getline(std::cin,line)||!stable()){std::cerr<<"Selection/session changed\n";return 4;}
  Snapshot s;s.stage=names[i];
  std::cout<<"Actual queue count: ";
  if(!std::getline(std::cin,line))return 4;
  char* end=nullptr;long n=std::strtol(line.c_str(),&end,10);
  if(end==line.c_str()||*end!='\0'||n<0||n>1000)return 4;
  s.observed=int(n);
  std::cout<<"Count visually confirmed (y/n): ";
  if(!std::getline(std::cin,line))return 4;
  s.verified=(line=="y"||line=="Y");
  if(!u32(process.h,tc.object,0x214,s.pointer)||
     !u32(process.h,tc.object,0x218,s.word218)||
     !u32(process.h,tc.object,0x21c,s.word21c)){std::cerr<<"TC fields unreadable\n";return 4;}
  if(s.pointer>=0x10000&&!(s.pointer&3u)&&s.pointer<=UINT32_MAX-window){
   s.bytes.resize(window);
   s.readable=rd(process.h,s.pointer,s.bytes.data(),window);
   if(!s.readable)s.bytes.clear();
  }
  if(!stable()){std::cerr<<"Selection changed during capture\n";return 4;}
  snapshots.push_back(std::move(s));
 }
 std::ofstream values("aoe2hd_production_queue_pointer_v3.csv");
 std::ofstream words("aoe2hd_production_queue_pointer_words_v3.csv");
 std::ofstream diffs("aoe2hd_production_queue_pointer_deltas_v3.csv");
 if(!values||!words||!diffs)return 5;
 values<<"stage,queue_observed,operator_confirmed,pointer_214,word_218,word_21c,low16_21c,high16_21c,pointer_readable,independent_verified\n";
 words<<"stage,pointer,offset,value,readable\n";
 diffs<<"from_stage,to_stage,same_pointer,offset,before,after,changed,comparison_valid\n";
 uint32_t changed=0,compared=0;
 for(size_t i=0;i<snapshots.size();++i){
  const auto& s=snapshots[i];
  auto split=aoe2hd::DecodeQueueWordV2(s.word21c);
  values<<s.stage<<','<<s.observed<<','<<s.verified<<','<<s.pointer<<','
        <<s.word218<<','<<s.word21c<<','<<split.low<<','<<split.high<<','
        <<s.readable<<",0\n";
  if(s.readable)for(uint32_t off=0;off<window;off+=4)
   words<<s.stage<<','<<s.pointer<<','<<off<<','
        <<aoe2hd::ReadWordV2(s.bytes,off)<<",1\n";
  if(i==0)continue;
  const auto& prev=snapshots[i-1];
  bool valid=prev.readable&&s.readable&&prev.pointer==s.pointer;
  if(!valid){
   diffs<<prev.stage<<','<<s.stage<<','<<(prev.pointer==s.pointer)
        <<",-1,0,0,0,0\n";
   continue;
  }
  for(uint32_t off=0;off<window;off+=4){
   auto before=aoe2hd::ReadWordV2(prev.bytes,off);
   auto after=aoe2hd::ReadWordV2(s.bytes,off);
   bool diff=before!=after;changed+=diff;++compared;
   if(diff)diffs<<prev.stage<<','<<s.stage<<",1,"<<off<<','
                 <<before<<','<<after<<",1,1\n";
  }
 }
 std::cout<<"ProductionQueueStructureDiscovery v3 tracked_stages="<<snapshots.size()
          <<" pointer_readable_stages="<<std::count_if(snapshots.begin(),snapshots.end(),[](const Snapshot& s){return s.readable;})
          <<" compared_words="<<compared<<" changed_words="<<changed<<"\n"
          <<"CSV: aoe2hd_production_queue_pointer_v3.csv\n"
          <<"CSV: aoe2hd_production_queue_pointer_words_v3.csv\n"
          <<"CSV: aoe2hd_production_queue_pointer_deltas_v3.csv\n"
          <<"Pointer may reference a queue, but no entry layout is confirmed.\n";
 return 0;
}

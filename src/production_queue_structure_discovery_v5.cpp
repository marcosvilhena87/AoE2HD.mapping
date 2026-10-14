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
 uint32_t id=pid(),base=id?module(id):0;
 if(!base){std::cerr<<"AoK HD.exe unavailable\n";return 2;}
 Handle proc(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,id));
 if(!proc.h)return 2;
 Anchor b;std::string error,line;
 if(!acquire(proc.h,base,b,error)){std::cerr<<"Select a Barracks first: "<<error<<"\n";return 3;}
 std::cout<<"Selected object ID="<<b.id<<" address=0x"<<std::hex<<b.object<<std::dec
          <<"\nConfirm BARRACKS with an empty production queue (y/n): ";
 if(!std::getline(std::cin,line)||(line!="y"&&line!="Y"))return 3;
 auto selected=[&](){
  uint32_t a=0,c=0,identity=0,w=0;
  return u32(proc.h,b.local,0x15c,a)&&u32(proc.h,b.local,0x160,c)&&
   a==b.object&&c==b.object&&u32(proc.h,b.object,8,identity)&&identity==b.id&&
   u32(proc.h,b.owner,0xc8,w)&&w==b.world;
 };
 struct Sample{std::string phase,observed;int count=0;bool verified=false;
   uint32_t pointer=0,word218=0,word21c=0;bool readable=false;
   std::vector<uint8_t> bytes;};
 std::vector<Sample> samples;
 struct Step{const char* phase;const char* prompt;};
 const Step steps[]={
  {"empty","Empty queue"},
  {"militia","Queue one Militia"},
  {"mixed2","Queue one Spearman after Militia"},
  {"mixed3","Queue another Militia after Spearman"},
  {"cancel","Cancel the last Militia order (if UI supports this)"},
  {"finish1","Wait for the first unit to finish"},
  {"finish2","Wait for the next unit to finish"}};
 constexpr uint32_t window=512;
 for(const auto& step:steps){
  std::cout<<step.prompt<<". Press ENTER to capture.\n";
  if(!std::getline(std::cin,line)||!selected()){std::cerr<<"Selection/session changed\n";return 4;}
  Sample s;s.phase=step.phase;
  std::cout<<"Visually observed queue order (e.g. Militia|Spearman|Militia, or empty): ";
  if(!std::getline(std::cin,s.observed))return 4;
  std::cout<<"Visually observed number of orders: ";
  if(!std::getline(std::cin,line))return 4;
  char* end=nullptr;long n=std::strtol(line.c_str(),&end,10);
  if(end==line.c_str()||*end!='\0'||n<0||n>1000)return 4;s.count=int(n);
  std::cout<<"Queue order and count both visually confirmed (y/n): ";
  if(!std::getline(std::cin,line))return 4;s.verified=line=="y"||line=="Y";
  if(!u32(proc.h,b.object,0x214,s.pointer)||!u32(proc.h,b.object,0x218,s.word218)||
     !u32(proc.h,b.object,0x21c,s.word21c)){std::cerr<<"Building unreadable\n";return 4;}
  if(s.pointer>=0x10000&&!(s.pointer&3u)&&s.pointer<=UINT32_MAX-window){
   s.bytes.resize(window);
   s.readable=rd(proc.h,s.pointer,s.bytes.data(),window);
   if(!s.readable)s.bytes.clear();
  }
  if(!selected()){std::cerr<<"Selection changed during capture\n";return 4;}
  samples.push_back(std::move(s));
 }
 std::ofstream stages("aoe2hd_production_queue_mixed_stages_v5.csv");
 std::ofstream words("aoe2hd_production_queue_mixed_words_v5.csv");
 std::ofstream deltas("aoe2hd_production_queue_mixed_deltas_v5.csv");
 if(!stages||!words||!deltas)return 5;
 stages<<"phase,observed_sequence,observed_count,operator_verified,pointer_214,word_218,word_21c,root_low16,root_high16,pointer_readable,entry_type_low16,entry_qty_high16,independently_verified\n";
 words<<"phase,pointer,offset,raw32,low16,high16\n";
 deltas<<"previous_phase,current_phase,same_pointer,comparable,offset,previous_raw32,current_raw32,changed\n";
 int readableCount=0,changes=0;bool rootMatch=true,entryMatch=true,allVerified=true;
 for(size_t i=0;i<samples.size();++i){
  const auto& s=samples[i];
  auto root=aoe2hd::DecodeQueueWordV2(s.word21c);
  auto entry=aoe2hd::DecodeQueueWordV2(s.readable?aoe2hd::ReadWordV2(s.bytes,0):0);
  readableCount+=s.readable;allVerified&=s.verified;
  stages<<s.phase<<",\""<<s.observed<<"\","<<s.count<<','<<s.verified<<','
        <<s.pointer<<','<<s.word218<<','<<s.word21c<<','
        <<root.low<<','<<root.high<<','<<s.readable<<','
        <<entry.low<<','<<entry.high<<",0\n";
  if(s.readable)for(uint32_t off=0;off<window;off+=4){
   auto word=aoe2hd::DecodeQueueWordV2(aoe2hd::ReadWordV2(s.bytes,off));
   words<<s.phase<<','<<s.pointer<<','<<off<<','
        <<aoe2hd::ReadWordV2(s.bytes,off)<<','<<word.low<<','<<word.high<<"\n";
  }
  if(i==0)continue;
  const auto& prev=samples[i-1];
  int dCount=s.count-prev.count;
  rootMatch &= int(root.low)-int(aoe2hd::DecodeQueueWordV2(prev.word21c).low)==dCount;
  if(prev.readable&&s.readable&&prev.pointer==s.pointer){
   entryMatch &= int(entry.high)-int(aoe2hd::DecodeQueueWordV2(aoe2hd::ReadWordV2(prev.bytes,0)).high)==dCount;
   for(uint32_t off=0;off<window;off+=4){
    uint32_t x=aoe2hd::ReadWordV2(prev.bytes,off),y=aoe2hd::ReadWordV2(s.bytes,off);
    if(x!=y){++changes;deltas<<prev.phase<<','<<s.phase<<",1,1,"<<off<<','<<x<<','<<y<<",1\n";}
   }
  }else{
   deltas<<prev.phase<<','<<s.phase<<','<<(prev.pointer==s.pointer)<<",0,-1,0,0,0\n";
  }
 }
 std::cout<<"ProductionQueueStructureDiscovery v5 stages="<<samples.size()
          <<" readable_stages="<<readableCount<<" changed_words="<<changes
          <<" root_count_matches="<<(allVerified&&rootMatch)
          <<" entry_count_matches_comparable="<<(allVerified&&entryMatch)<<"\n"
          <<"CSV: aoe2hd_production_queue_mixed_stages_v5.csv\n"
          <<"CSV: aoe2hd_production_queue_mixed_words_v5.csv\n"
          <<"CSV: aoe2hd_production_queue_mixed_deltas_v5.csv\n"
          <<"Operator observations only. Mixed-order layout is not independently confirmed.\n";
 return 0;
}

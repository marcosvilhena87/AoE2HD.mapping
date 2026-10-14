#include "UnitReferenceDiscovery.h"
#include "UnitCollectionDiscovery.h"
#include "UnitIdentityDiscovery.h"
#include "EntityGraphDiscovery.h"
#include "AdaptiveEntityGraph.h"
#include "StructuralGraphEvidence.h"
#include "AnchorPersistenceScan.h"
#include "EntityGraphCoverageV5.h"
#include "UnitArrayCandidateValidator.h"
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


int main(int argc,char** argv){
 uint32_t maxMiB=128;
 for(int i=1;i<argc;++i){
  if(std::string(argv[i])!="--max-mib"||i+1>=argc){std::cerr<<"Usage: --max-mib 1..512\n";return 2;}
  char* end=nullptr;unsigned long n=std::strtoul(argv[++i],&end,10);
  if(!end||*end||n<1||n>512){std::cerr<<"Invalid max-mib\n";return 2;}
  maxMiB=uint32_t(n);
 }
 uint32_t id=pid(),base=id?module(id):0;
 if(!base){std::cerr<<"AoK HD.exe unavailable\n";return 2;}
 Handle h(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,id));
 if(!h.h){std::cerr<<"OpenProcess failed\n";return 2;}
 Anchor a,b;std::string why;
 if(!acquire(h.h,base,a,why)){std::cerr<<"First anchor: "<<why<<"\n";return 3;}
 std::cout<<"First villager 0x"<<std::hex<<a.object<<std::dec<<" id="<<a.id
          <<"\nSelect a DIFFERENT villager and press ENTER.\n";
 std::string line;std::getline(std::cin,line);
 if(!acquire(h.h,base,b,why)||b.world!=a.world||b.local!=a.local||
    a.object==b.object||a.id==b.id){std::cerr<<"Second anchor invalid\n";return 3;}
 uint32_t firstId=0;
 if(!u32(h.h,a.object,8,firstId)||firstId!=a.id){std::cerr<<"First ID changed\n";return 3;}
 auto rdFn=[&](uint32_t p,void* dest,size_t n){return rd(h.h,p,dest,n);};
 auto map=enumerate(h.h,b);
 std::vector<uint32_t> firstSlots,secondSlots;
 std::vector<uint8_t> buffer(65536);
 uint64_t attempted=0,readBytes=0;
 const uint64_t budget=uint64_t(maxMiB)*1024*1024;
 for(const auto& region:map){
  if(attempted>=budget)break;
  for(uint64_t off=0;off<region.size&&attempted<budget;){
   const size_t n=size_t(std::min<uint64_t>({buffer.size(),uint64_t(region.size)-off,budget-attempted}));
   if(n<4)break;
   uint32_t at=region.base+uint32_t(off);SIZE_T got=0;
   BOOL ok=ReadProcessMemory(h.h,reinterpret_cast<LPCVOID>(uintptr_t(at)),buffer.data(),n,&got);
   attempted+=n;
   if(ok&&got>=4){
    readBytes+=got;
    // No early limit on scanning by anchor hit count; coverage budget governs.
    for(size_t p=0;p+4<=got;p+=4){
     uint32_t value=0;std::memcpy(&value,buffer.data()+p,4);
     if(value==a.object)firstSlots.push_back(at+uint32_t(p));
     if(value==b.object)secondSlots.push_back(at+uint32_t(p));
    }
   }
   off+=n;
  }
 }
 Anchor verify;std::string reason;
 if(!acquire(h.h,base,verify,reason)||verify.object!=b.object||
    verify.id!=b.id||verify.world!=a.world||verify.local!=a.local||
    !u32(h.h,a.object,8,firstId)||firstId!=a.id){
  std::cerr<<"Selection or identity changed during search\n";return 4;
 }
 const auto pairs=aoe2hd::FindEightBytePairs(firstSlots,secondSlots);
 std::cout<<"Validator v1 first_hits="<<firstSlots.size()
          <<" second_hits="<<secondSlots.size()<<" pairs="<<pairs.size()
          <<" attempted="<<attempted<<" read="<<readBytes<<"\n";
 // A "pair" might be two fields in one record or adjacent array entries.
 // Sample a bounded neighborhood and keep both raw words for each entry.
 std::ofstream csv("aoe2hd_unit_array_candidates_v1.csv",std::ios::trunc);
 if(!csv){std::cerr<<"CSV open failed\n";return 5;}
 csv<<"pair_index,first_slot,second_slot,stride,entry_slot,entry_value,entry_meta,candidate_id,hp,plausible,selection_slot,world_direct,local_direct,status\n";
 auto players=aoe2hd::ReadPlayerRoots(rdFn,a.world);
 auto referenced=[&](uint32_t root,uint32_t target){
  for(uint32_t o=0;o<0x800;o+=4){uint32_t w=0;
   if(u32(h.h,root,o,w)&&w==target)return true;
  }
  return false;
 };
 std::set<uint32_t> candidates;
 for(size_t i=0;i<pairs.size();++i){
  const uint32_t left=std::min(pairs[i].firstSlot,pairs[i].secondSlot);
  if(left<64||left>UINT32_MAX-128)continue;
  candidates.insert(left);
  for(int j=-4;j<=5;++j){
   uint32_t slot=uint32_t(int64_t(left)+int64_t(j)*8);
   auto e=aoe2hd::InspectEntry(rdFn,slot);
   csv<<i<<','<<pairs[i].firstSlot<<','<<pairs[i].secondSlot<<",8,"
      <<slot<<','<<e.value<<','<<e.meta<<','<<e.id<<','<<e.hp<<','
      <<e.plausible<<','
      <<(slot==a.local+0x15c||slot==a.local+0x160)<<','
      <<referenced(a.world,left)<<','<<referenced(a.local,left)
      <<",hypothesis\n";
  }
 }
 std::ofstream roots("aoe2hd_unit_array_root_refs_v1.csv",std::ios::trunc);
 if(!roots){std::cerr<<"Root CSV open failed\n";return 5;}
 roots<<"candidate,root,root_kind,field_offset,field_value,kind,status\n";
 for(uint32_t candidate:candidates){
  std::vector<std::pair<uint32_t,std::string>> rootsList{{a.world,"World"},{a.local,"LocalPlayer"}};
  for(uint32_t player:players)rootsList.push_back({player,"Player"});
  for(const auto& root:rootsList)for(uint32_t o=0;o<0x800;o+=4){
   uint32_t word=0;
   if(!u32(h.h,root.first,o,word))continue;
   if(word==candidate||word==candidate-32||word==candidate-64){
    roots<<candidate<<','<<root.first<<','<<root.second<<','<<o<<','
         <<word<<",direct_or_near,hypothesis\n";
   }
  }
 }
 std::cout<<"Candidate CSV: aoe2hd_unit_array_candidates_v1.csv\n"
          <<"Root CSV: aoe2hd_unit_array_root_refs_v1.csv\n"
          <<"Optional controlled test: create ONE new villager WITHOUT selecting it, then press ENTER.\n";
 std::getline(std::cin,line);
 Anchor after;std::string afterWhy;
 if(!acquire(h.h,base,after,afterWhy)||after.object!=b.object||
    after.world!=a.world||after.local!=a.local||
    !u32(h.h,a.object,8,firstId)||firstId!=a.id){
  std::cerr<<"Selection changed or object invalid after test; no delta written\n";return 6;
 }
 std::ofstream delta("aoe2hd_unit_array_delta_v1.csv",std::ios::trunc);
 if(!delta){std::cerr<<"Delta CSV open failed\n";return 5;}
 delta<<"candidate,slot,after_pointer,after_meta,after_id,after_hp,after_plausible,status\n";
 uint32_t newIds=0;
 for(uint32_t candidate:candidates){
  for(int j=-4;j<=5;++j){
   uint32_t slot=uint32_t(int64_t(candidate)+int64_t(j)*8);
   auto entry=aoe2hd::InspectEntry(rdFn,slot);
   if(entry.plausible&&entry.id!=a.id&&entry.id!=b.id)++newIds;
   delta<<candidate<<','<<slot<<','<<entry.value<<','<<entry.meta<<','
        <<entry.id<<','<<entry.hp<<','<<entry.plausible<<",hypothesis\n";
  }
 }
 std::cout<<"Delta CSV: aoe2hd_unit_array_delta_v1.csv; other_plausible_entries="
          <<newIds<<"\nNo array confirmed without structural and temporal validation.\n";
 return 0;
}
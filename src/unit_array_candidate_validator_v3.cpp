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
 // Prioritize any region containing World, LocalPlayer, known players, or
 // either anchor. Keep the rest in the enumeration for honest coverage.
 auto players=aoe2hd::ReadPlayerRoots(rdFn,a.world);
 std::set<uint32_t> priority{a.world,a.local,a.object,b.object};
 priority.insert(players.begin(),players.end());
 std::stable_sort(map.begin(),map.end(),[&](const ScanRegion& x,const ScanRegion& y){
  auto rank=[&](const ScanRegion& r){
   uint32_t n=0;
   for(auto p:priority)
    if(p>=r.base&&uint64_t(p)<uint64_t(r.base)+r.size)++n;
   return n;
  };
  return rank(x)>rank(y);
 });
 std::vector<uint32_t> firstSlots,secondSlots;
 std::vector<uint8_t> buffer(65536);
 uint64_t attempted=0,readBytes=0;
 struct Coverage {uint32_t base,size;uint64_t attempted=0,read=0;uint32_t failures=0;};
 std::vector<Coverage> coverage;
 const uint64_t budget=uint64_t(maxMiB)*1024*1024;
 for(const auto& region:map){
  if(attempted>=budget)break;
  Coverage cov{region.base,region.size};
  for(uint64_t off=0;off<region.size&&attempted<budget;){
   const size_t n=size_t(std::min<uint64_t>({buffer.size(),uint64_t(region.size)-off,budget-attempted}));
   if(n<4)break;
   uint32_t at=region.base+uint32_t(off);SIZE_T got=0;
   BOOL ok=ReadProcessMemory(h.h,reinterpret_cast<LPCVOID>(uintptr_t(at)),buffer.data(),n,&got);
   attempted+=n;cov.attempted+=n;
   if(ok&&got>=4){
    cov.read+=got;
    readBytes+=got;
    // No early limit on scanning by anchor hit count; coverage budget governs.
    for(size_t p=0;p+4<=got;p+=4){
     uint32_t value=0;std::memcpy(&value,buffer.data()+p,4);
     if(value==a.object)firstSlots.push_back(at+uint32_t(p));
     if(value==b.object)secondSlots.push_back(at+uint32_t(p));
    }
   }else ++cov.failures;
   off+=n;
  }
  coverage.push_back(cov);
 }
 Anchor verify;std::string reason;
 if(!acquire(h.h,base,verify,reason)||verify.object!=b.object||
    verify.id!=b.id||verify.world!=a.world||verify.local!=a.local||
    !u32(h.h,a.object,8,firstId)||firstId!=a.id){
  std::cerr<<"Selection or identity changed during search\n";return 4;
 }
 const auto pairs=aoe2hd::FindEightBytePairs(firstSlots,secondSlots);
 // Single-anchor slots are retained as hypotheses for subsequent comparison.
 std::set<uint32_t> singleSlots(firstSlots.begin(),firstSlots.end());
 singleSlots.insert(secondSlots.begin(),secondSlots.end());
 singleSlots.erase(a.local+0x15c);singleSlots.erase(a.local+0x160);
 std::cout<<"Validator v3 first_hits="<<firstSlots.size()
          <<" second_hits="<<secondSlots.size()<<" pairs="<<pairs.size()
          <<" attempted="<<attempted<<" read="<<readBytes<<"\n";
 // A "pair" might be two fields in one record or adjacent array entries.
 // Sample a bounded neighborhood and keep both raw words for each entry.
 std::ofstream csv("aoe2hd_unit_array_candidates_v3.csv",std::ios::trunc);
 if(!csv){std::cerr<<"CSV open failed\n";return 5;}
 csv<<"pair_index,first_slot,second_slot,stride,entry_slot,entry_value,entry_meta,candidate_id,hp,plausible,selection_slot,world_direct,local_direct,status\n";
 auto referenced=[&](uint32_t root,uint32_t target){
  for(uint32_t o=0;o<0x800;o+=4){uint32_t w=0;
   if(u32(h.h,root,o,w)&&w==target)return true;
  }
  return false;
 };
 std::set<uint32_t> candidates;
 std::map<uint32_t,aoe2hd::EntrySnapshot> before;
 for(uint32_t slot:singleSlots){
  if(slot<64||slot>UINT32_MAX-128)continue;
  // Capture neighbors even without an observed second anchor.
  for(int j=-2;j<=2;++j){
   const uint32_t at=uint32_t(int64_t(slot)+int64_t(j)*8);
   before.emplace(at,aoe2hd::InspectEntry(rdFn,at));
  }
 }
 for(size_t i=0;i<pairs.size();++i){
  const uint32_t left=std::min(pairs[i].firstSlot,pairs[i].secondSlot);
  if(left<64||left>UINT32_MAX-128)continue;
  candidates.insert(left);
  for(int j=-4;j<=5;++j){
   uint32_t slot=uint32_t(int64_t(left)+int64_t(j)*8);
   auto e=aoe2hd::InspectEntry(rdFn,slot);
   before[slot]=e;
   csv<<i<<','<<pairs[i].firstSlot<<','<<pairs[i].secondSlot<<",8,"
      <<slot<<','<<e.value<<','<<e.meta<<','<<e.id<<','<<e.hp<<','
      <<e.plausible<<','
      <<(slot==a.local+0x15c||slot==a.local+0x160)<<','
      <<referenced(a.world,left)<<','<<referenced(a.local,left)
      <<",hypothesis\n";
  }
 }
 std::ofstream roots("aoe2hd_unit_array_root_refs_v3.csv",std::ios::trunc);
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
 std::cout<<"Candidate CSV: aoe2hd_unit_array_candidates_v3.csv\n"
          <<"Root CSV: aoe2hd_unit_array_root_refs_v3.csv\n"
          <<"Optional controlled test: create ONE new villager WITHOUT selecting it, then press ENTER.\n";
 std::getline(std::cin,line);
 // Selection is allowed to change to the Town Center during creation.
 // Validate World and both original identities directly, not selected slots.
 uint32_t ownerNow=0,worldNow=0,idA=0,idB=0;
 if(!u32(h.h,base,0x006F2C58,ownerNow)||ownerNow!=a.owner||
    !u32(h.h,a.owner,0xC8,worldNow)||worldNow!=a.world||
    !u32(h.h,a.object,8,idA)||idA!=a.id||
    !u32(h.h,b.object,8,idB)||idB!=b.id){
  std::cerr<<"World or original object identity changed; delta unsafe\n";return 6;
 }
 std::ofstream cov("aoe2hd_unit_array_coverage_v3.csv",std::ios::trunc);
 if(!cov){std::cerr<<"Coverage CSV open failed\n";return 5;}
 cov<<"region_base,region_size,attempted_bytes,readable_bytes,read_failures,status\n";
 for(const auto& c:coverage)
  cov<<c.base<<','<<c.size<<','<<c.attempted<<','<<c.read<<','
     <<c.failures<<','<<(c.attempted==c.size?"attempted_full":"partial")<<"\n";
 std::ofstream delta("aoe2hd_unit_array_delta_v3.csv",std::ios::trunc);
 if(!delta){std::cerr<<"Delta CSV open failed\n";return 5;}
 delta<<"slot,before_pointer,after_pointer,before_meta,after_meta,before_id,after_id,before_hp,after_hp,before_plausible,after_plausible,changed,status\n";
 // Rescan the same prioritized region order AFTER creation. Each phase has
 // an independent byte budget; newly discovered pointers are not restricted
 // to addresses found before creation.
 std::vector<uint32_t> afterFirst,afterSecond;
 uint64_t afterAttempted=0,afterRead=0;
 std::vector<Coverage> afterCoverage;
 for(const auto& region:map){
  if(afterAttempted>=budget)break;
  Coverage c{region.base,region.size};
  for(uint64_t off=0;off<region.size&&afterAttempted<budget;){
   size_t n=size_t(std::min<uint64_t>(
     {buffer.size(),uint64_t(region.size)-off,budget-afterAttempted}));
   if(n<4)break;
   uint32_t at=region.base+uint32_t(off);SIZE_T got=0;
   BOOL ok=ReadProcessMemory(h.h,reinterpret_cast<LPCVOID>(uintptr_t(at)),
                              buffer.data(),n,&got);
   afterAttempted+=n;c.attempted+=n;
   if(ok&&got>=4){
    afterRead+=got;c.read+=got;
    for(size_t p=0;p+4<=got;p+=4){
     uint32_t value=0;std::memcpy(&value,buffer.data()+p,4);
     if(value==a.object)afterFirst.push_back(at+uint32_t(p));
     if(value==b.object)afterSecond.push_back(at+uint32_t(p));
    }
   }else ++c.failures;
   off+=n;
  }
  afterCoverage.push_back(c);
 }
 std::ofstream secondCov("aoe2hd_unit_array_coverage_after_v3.csv");
 if(!secondCov){std::cerr<<"After coverage CSV failed\n";return 5;}
 secondCov<<"region_base,region_size,attempted_bytes,readable_bytes,read_failures,status\n";
 for(const auto& c:afterCoverage)
  secondCov<<c.base<<','<<c.size<<','<<c.attempted<<','<<c.read<<','
           <<c.failures<<','<<(c.attempted==c.size?"attempted_full":"partial")<<"\n";
 const auto afterPairs=aoe2hd::FindEightBytePairs(afterFirst,afterSecond);
 std::ofstream newHits("aoe2hd_unit_array_rescan_v3.csv");
 if(!newHits){std::cerr<<"Rescan CSV failed\n";return 5;}
 newHits<<"phase,anchor,slot,paired_eight_bytes,status\n";
 auto emit=[&](const std::vector<uint32_t>& slots,
              const std::vector<aoe2hd::ArrayPair>& found,
              const char* phase,const char* role){
  for(uint32_t slot:slots){
   bool paired=false;
   for(const auto& pair:found)
    if(pair.firstSlot==slot||pair.secondSlot==slot){paired=true;break;}
   newHits<<phase<<','<<role<<','<<slot<<','<<paired<<",hypothesis\n";
  }
 };
 emit(firstSlots,pairs,"before","A");emit(secondSlots,pairs,"before","B");
 emit(afterFirst,afterPairs,"after","A");emit(afterSecond,afterPairs,"after","B");
 uint32_t newIds=0,changedCount=0;
 for(const auto& item:before){
  const uint32_t slot=item.first;
  const auto& old=item.second;
  auto entry=aoe2hd::InspectEntry(rdFn,slot);
  bool changed=old.value!=entry.value||old.meta!=entry.meta||
    old.id!=entry.id||old.plausible!=entry.plausible||
    (old.plausible&&entry.plausible&&old.hp!=entry.hp);
  changedCount+=changed;
  if(entry.plausible&&entry.id!=a.id&&entry.id!=b.id)++newIds;
  delta<<slot<<','<<old.value<<','<<entry.value<<','
       <<old.meta<<','<<entry.meta<<','<<old.id<<','<<entry.id<<','
       <<old.hp<<','<<entry.hp<<','<<old.plausible<<','
       <<entry.plausible<<','<<changed<<",hypothesis\n";
 }
 std::cout<<"Delta CSV: aoe2hd_unit_array_delta_v3.csv; other_plausible_entries="
          <<newIds<<" changed_entries="<<changedCount
          <<" after_first="<<afterFirst.size()<<" after_second="<<afterSecond.size()
          <<" after_pairs="<<afterPairs.size()<<"\nRescan CSV: aoe2hd_unit_array_rescan_v3.csv\nCoverage CSV: aoe2hd_unit_array_coverage_v3.csv\nNo array confirmed without structural and temporal validation.\n";
 return 0;
}
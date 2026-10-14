#include "UnitReferenceDiscovery.h"
#include "UnitCollectionDiscovery.h"
#include "UnitIdentityDiscovery.h"
#include "EntityGraphDiscovery.h"
#include "AdaptiveEntityGraph.h"
#include "StructuralGraphEvidence.h"
#include "AnchorPersistenceScan.h"
#include "EntityGraphCoverageV5.h"
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
 uint32_t maxMiB=128,perAnchor=2048;
 for(int i=1;i<argc;++i){
  std::string opt=argv[i];if(i+1>=argc){std::cerr<<"Missing value\n";return 2;}
  char* end=nullptr;unsigned long v=std::strtoul(argv[++i],&end,10);
  if(!end||*end||!v){std::cerr<<"Invalid value\n";return 2;}
  if(opt=="--max-mib"&&v<=512)maxMiB=uint32_t(v);
  else if(opt=="--per-anchor"&&v<=8192)perAnchor=uint32_t(v);
  else{std::cerr<<"Invalid option\n";return 2;}
 }
 const uint32_t id=pid(),base=id?module(id):0;
 if(!base){std::cerr<<"AoK HD.exe unavailable\n";return 2;}
 Handle h(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,id));
 if(!h.h){std::cerr<<"OpenProcess failed\n";return 2;}
 Anchor a,b;std::string why;
 if(!acquire(h.h,base,a,why)){std::cerr<<"First anchor: "<<why<<"\n";return 3;}
 std::cout<<"First anchor 0x"<<std::hex<<a.object<<std::dec<<" id="<<a.id
          <<"\nSelect a DIFFERENT villager and press ENTER.\n";
 std::string line;std::getline(std::cin,line);
 if(!acquire(h.h,base,b,why)||b.world!=a.world||b.local!=a.local||
    a.object==b.object||a.id==b.id){std::cerr<<"Second anchor invalid\n";return 3;}
 const auto reader=[&](uint32_t at,void* dst,size_t n){return rd(h.h,at,dst,n);};
 uint32_t originalId=0;
 if(!u32(h.h,a.object,8,originalId)||originalId!=a.id){std::cerr<<"First ID unstable\n";return 3;}
 // Player objects are structural roots; World+0x184 stores known player entries.
 const auto players=aoe2hd::ReadPlayerRoots(reader,a.world);
 std::set<uint32_t> roots{a.world,a.local};
 roots.insert(players.begin(),players.end());
 std::ofstream rootsCsv("aoe2hd_entity_roots_v5.csv");
 if(!rootsCsv){std::cerr<<"Roots CSV failed\n";return 5;}
 rootsCsv<<"address,kind,first_anchor,second_anchor,status\n";
 for(auto r:roots)rootsCsv<<r<<','<<(r==a.world?"World":r==a.local?"LocalPlayer":"Player")
                     <<','<<a.object<<','<<b.object<<",hypothesis\n";
 rootsCsv.close();
 auto mapped=enumerate(h.h,b);
 // Prefer memory regions containing player objects, the World and both anchors.
 std::stable_sort(mapped.begin(),mapped.end(),[&](const ScanRegion& x,const ScanRegion& y){
  auto score=[&](const ScanRegion& r){
   int count=0;
   for(auto p:roots)if(p>=r.base&&uint64_t(p)<uint64_t(r.base)+r.size)++count;
   if(a.object>=r.base&&uint64_t(a.object)<uint64_t(r.base)+r.size)++count;
   if(b.object>=r.base&&uint64_t(b.object)<uint64_t(r.base)+r.size)++count;
   return count;
  };
  return score(x)>score(y);
 });
 std::vector<aoe2hd::AnchorReference> ah,bh;
 std::vector<aoe2hd::CoverageRegion> coverage;
 std::vector<uint8_t> buf(65536);
 const uint64_t budget=uint64_t(maxMiB)*1024*1024;
 uint64_t attempted=0,actualRead=0;
 for(const auto& reg:mapped){
  aoe2hd::CoverageRegion cr{reg.base,reg.size,0,0,0};
  for(uint64_t off=0;off<reg.size&&attempted<budget;){
   if(ah.size()>=perAnchor&&bh.size()>=perAnchor)break;
   const size_t n=size_t(std::min<uint64_t>({buf.size(),uint64_t(reg.size)-off,budget-attempted}));
   if(n<4)break;
   uint32_t at=reg.base+uint32_t(off);
   SIZE_T got=0;
   BOOL ok=ReadProcessMemory(h.h,reinterpret_cast<LPCVOID>(uintptr_t(at)),buf.data(),n,&got);
   attempted+=n;cr.attempted+=n;
   if(ok&&got>=4){
    cr.read+=got;actualRead+=got;
    aoe2hd::CollectAnchorReferences(at,reg.base,buf.data(),got,
      a.object,b.object,ah,bh,perAnchor);
   }else ++cr.errors;
   off+=n;
  }
  coverage.push_back(cr);
  if(attempted>=budget||(ah.size()>=perAnchor&&bh.size()>=perAnchor))break;
 }
 Anchor still;std::string rs;
 if(!acquire(h.h,base,still,rs)||still.object!=b.object||
    still.world!=a.world||still.local!=a.local||still.id!=b.id||
    !u32(h.h,a.object,8,originalId)||originalId!=a.id){
  std::cerr<<"Session changed during search\n";return 4;
 }
 std::cout<<"V5 player_roots="<<players.size()<<" first_hits="<<ah.size()
          <<" second_hits="<<bh.size()<<" attempted_bytes="<<attempted
          <<" readable_bytes="<<actualRead<<"\n"
          <<"Select the FIRST villager again and press ENTER.\n";
 std::getline(std::cin,line);
 Anchor c;std::string rc;
 if(!acquire(h.h,base,c,rc)||c.object!=a.object||c.id!=a.id||
    c.world!=a.world||c.local!=a.local||
    !u32(h.h,b.object,8,originalId)||originalId!=b.id){
  std::cerr<<"Expected return to first villager; no results exported\n";return 4;
 }
 std::ofstream references("aoe2hd_entity_references_v5.csv");
 if(!references){std::cerr<<"References CSV failed\n";return 5;}
 references<<"address,region,initial_anchor,initial_target,selection_slot,readable_after,observed_after,unchanged,switched_to_first,status\n";
 uint32_t stableA=0,stableB=0,switched=0,independent=0;
 auto report=[&](const std::vector<aoe2hd::AnchorReference>& hits,const char* label){
  for(const auto& hit:hits){
   uint32_t now=0;bool ok=rd(h.h,hit.address,&now,4);
   bool unchanged=aoe2hd::IsStableReference(ok,hit.value,now);
   bool selected=hit.address==a.local+0x15c||hit.address==a.local+0x160;
   bool changed=aoe2hd::IsSwitchingReference(ok,hit.value,now,a.object,b.object);
   stableA+=unchanged&&hit.value==a.object;
   stableB+=unchanged&&hit.value==b.object;
   switched+=changed;
   independent+=unchanged&&!selected;
   references<<hit.address<<','<<hit.region<<','<<label<<','<<hit.value<<','
             <<selected<<','<<ok<<',';
   if(ok)references<<now;
   references<<','<<unchanged<<','<<changed<<",hypothesis\n";
  }
 };
 report(ah,"first");report(bh,"second");
 std::ofstream cov("aoe2hd_entity_coverage_v5.csv");
 if(!cov){std::cerr<<"Coverage CSV failed\n";return 5;}
 cov<<"region_base,region_size,attempted_bytes,readable_bytes,read_errors,coverage_status\n";
 for(const auto& cr:coverage)
  cov<<cr.base<<','<<cr.size<<','<<cr.attempted<<','<<cr.read<<','
     <<cr.errors<<','<<(cr.attempted==cr.size?"attempted_full":"partial")<<"\n";
 std::ofstream summary("aoe2hd_entity_summary_v5.csv");
 if(!summary){std::cerr<<"Summary CSV failed\n";return 5;}
 summary<<"pid,world,local,players,regions_examined,regions_available,attempted_bytes,readable_bytes,first_hits,second_hits,unchanged_first,unchanged_second,switched_second_to_first,unchanged_outside_selection,both_anchors_seen,budget_exhausted,status\n";
 summary<<id<<','<<a.world<<','<<a.local<<','<<players.size()<<','
        <<coverage.size()<<','<<mapped.size()<<','<<attempted<<','<<actualRead<<','
        <<ah.size()<<','<<bh.size()<<','<<stableA<<','<<stableB<<','
        <<switched<<','<<independent<<','
        <<aoe2hd::BothAnchorsObserved(uint32_t(ah.size()),uint32_t(bh.size()))<<','
        <<(attempted>=budget)<<",hypothesis\n";
 std::cout<<"V5 unchanged_first="<<stableA<<" unchanged_second="<<stableB
          <<" switched_second_to_first="<<switched
          <<" unchanged_outside_selection="<<independent<<"\n"
          <<"CSV: aoe2hd_entity_references_v5.csv\n"
          <<"CSV: aoe2hd_entity_coverage_v5.csv\n"
          <<"CSV: aoe2hd_entity_roots_v5.csv\n"
          <<"CSV: aoe2hd_entity_summary_v5.csv\n"
          <<"No collection is confirmed by isolated references.\n";
 return 0;
}

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
  if(!end||*end||n<1||n>512)return 2;maxMiB=uint32_t(n);
 }
 const uint32_t id=pid(),base=id?module(id):0;
 if(!base){std::cerr<<"AoK HD.exe unavailable\n";return 2;}
 Handle h(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,id));
 if(!h.h)return 2;
 Anchor a,b,c;std::string reason,line;
 if(!acquire(h.h,base,a,reason)){std::cerr<<reason<<"\n";return 3;}
 auto reader=[&](uint32_t at,void* out,size_t n){return rd(h.h,at,out,n);};
 const auto players=aoe2hd::ReadPlayerRoots(reader,a.world);
 auto regions=enumerate(h.h,a);
 std::set<uint32_t> important{a.world,a.local,a.object};
 important.insert(players.begin(),players.end());
 // Use one fixed scan order and byte budget across phases.
 std::stable_sort(regions.begin(),regions.end(),[&](const ScanRegion& x,const ScanRegion& y){
  auto count=[&](const ScanRegion& r){
   unsigned v=0;for(uint32_t p:important)
    if(p>=r.base&&uint64_t(p)<uint64_t(r.base)+r.size)++v;
   return v;
  };
  return count(x)>count(y);
 });
 struct Sample{uint32_t address=0,value=0;};
 struct Phase{std::vector<Sample> hits;uint64_t attempted=0,read=0;};
 const uint64_t budget=uint64_t(maxMiB)*1024*1024;
 auto scan=[&](uint32_t first,uint32_t second){
  Phase phase;std::vector<uint8_t> buf(65536);
  for(const auto& region:regions){
   if(phase.attempted>=budget)break;
   for(uint64_t off=0;off<region.size&&phase.attempted<budget;){
    const size_t n=size_t(std::min<uint64_t>(
     {buf.size(),uint64_t(region.size)-off,budget-phase.attempted}));
    if(n<4)break;
    const uint32_t at=region.base+uint32_t(off);SIZE_T got=0;
    BOOL ok=ReadProcessMemory(h.h,reinterpret_cast<LPCVOID>(uintptr_t(at)),buf.data(),n,&got);
    phase.attempted+=n;
    if(ok&&got>=4){
     phase.read+=got;
     for(size_t p=0;p+4<=got;p+=4){
      uint32_t v=0;std::memcpy(&v,buf.data()+p,4);
      if(v==first||(second&&v==second))
       phase.hits.push_back({at+uint32_t(p),v});
     }
    }
    off+=n;
   }
  }
  return phase;
 };
 std::cout<<"A=0x"<<std::hex<<a.object<<std::dec<<" id="<<a.id
          <<"\nScanning A selection...\n";
 // Phase A scans while A selected; phase B searches both A and B.
 auto before=scan(a.object,0);
 std::cout<<"A hits="<<before.hits.size()
          <<"\nSelect a DIFFERENT villager B and press ENTER.\n";
 std::getline(std::cin,line);
 if(!acquire(h.h,base,b,reason)||b.object==a.object||b.id==a.id||
    b.world!=a.world||b.local!=a.local){std::cerr<<"Invalid B\n";return 3;}
 uint32_t check=0;
 if(!u32(h.h,a.object,8,check)||check!=a.id)return 4;
 auto during=scan(a.object,b.object);
 std::cout<<"B scan hits="<<during.hits.size()
          <<"\nSelect villager A again and press ENTER.\n";
 std::getline(std::cin,line);
 if(!acquire(h.h,base,c,reason)||c.object!=a.object||c.id!=a.id||
    c.world!=a.world||c.local!=a.local||
    !u32(h.h,b.object,8,check)||check!=b.id){
  std::cerr<<"Return to A required\n";return 4;
 }
 auto after=scan(a.object,b.object);
 std::map<uint32_t,uint32_t> aVals,bVals,cVals;
 for(auto x:before.hits)aVals[x.address]=x.value;
 for(auto x:during.hits)bVals[x.address]=x.value;
 for(auto x:after.hits)cVals[x.address]=x.value;
 std::set<uint32_t> addresses;
 for(auto x:aVals)addresses.insert(x.first);
 for(auto x:bVals)addresses.insert(x.first);
 for(auto x:cVals)addresses.insert(x.first);
 std::ofstream refs("aoe2hd_unit_array_references_v4.csv");
 if(!refs)return 5;
 refs<<"address,phase_a,phase_b,phase_return_a,selection_slot,class,status\n";
 std::vector<uint32_t> stableA,stableB;
 uint32_t switching=0;
 for(uint32_t addr:addresses){
  // Values missing from a phase are read now only as a diagnostic; never
  // interpret a missing scan hit as evidence of a zero pointer.
  auto v=[&](const std::map<uint32_t,uint32_t>& m){
   auto i=m.find(addr);return i==m.end()?uint32_t(0):i->second;
  };
  uint32_t x=v(aVals),y=v(bVals),z=v(cVals);
  bool sel=addr==a.local+0x15c||addr==a.local+0x160;
  const bool stableFirst=x==a.object&&y==a.object&&z==a.object;
  const bool stableSecond=y==b.object&&z==b.object;
  const bool follows=x==a.object&&y==b.object&&z==a.object;
  if(stableFirst&&!sel)stableA.push_back(addr);
  if(stableSecond&&!sel)stableB.push_back(addr);
  switching+=follows;
  const char* cls=follows?"selection_follower":stableFirst?"stable_A":
                 stableSecond?"stable_B":"unclassified";
  refs<<addr<<','<<x<<','<<y<<','<<z<<','<<sel<<','<<cls<<",hypothesis\n";
 }
 std::ofstream pairs("aoe2hd_unit_array_pairs_v4.csv");
 if(!pairs)return 5;
 pairs<<"first_slot,second_slot,distance,first_id,second_id,status\n";
 uint32_t count=0;
 for(uint32_t x:stableA)for(uint32_t y:stableB){
  uint64_t dist=x>y?uint64_t(x)-y:uint64_t(y)-x;
  if(dist==8){pairs<<x<<','<<y<<",8,"<<a.id<<','<<b.id<<",hypothesis\n";++count;}
 }
 std::ofstream summary("aoe2hd_unit_array_summary_v4.csv");
 if(!summary)return 5;
 summary<<"pid,world,local,A,B,phaseA_attempted,phaseB_attempted,phaseReturn_attempted,phaseA_read,phaseB_read,phaseReturn_read,phaseA_hits,phaseB_hits,phaseReturn_hits,stable_A,stable_B,selection_followers,pairs,status\n";
 summary<<id<<','<<a.world<<','<<a.local<<','<<a.object<<','<<b.object<<','
        <<before.attempted<<','<<during.attempted<<','<<after.attempted<<','
        <<before.read<<','<<during.read<<','<<after.read<<','
        <<before.hits.size()<<','<<during.hits.size()<<','<<after.hits.size()<<','
        <<stableA.size()<<','<<stableB.size()<<','<<switching<<','<<count<<",hypothesis\n";
 std::cout<<"Validator v4 stable_A="<<stableA.size()
          <<" stable_B="<<stableB.size()<<" follows_selection="<<switching
          <<" stride8_pairs="<<count<<"\n"
          <<"CSV: aoe2hd_unit_array_references_v4.csv\n"
          <<"CSV: aoe2hd_unit_array_pairs_v4.csv\n"
          <<"CSV: aoe2hd_unit_array_summary_v4.csv\n"
          <<"No unit array confirmed.\n";
 return 0;
}

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
 const uint32_t pidValue=pid(),base=pidValue?module(pidValue):0;
 if(!base){std::cerr<<"AoK HD.exe unavailable\n";return 2;}
 Handle process(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,pidValue));
 if(!process.h){std::cerr<<"OpenProcess failed\n";return 2;}
 Anchor a,b;std::string why,line;
 if(!acquire(process.h,base,a,why)){std::cerr<<"First villager: "<<why<<"\n";return 3;}
 std::cout<<"A=0x"<<std::hex<<a.object<<std::dec<<" id="<<a.id
          <<"\nSelect a different villager B and press ENTER.\n";
 std::getline(std::cin,line);
 if(!acquire(process.h,base,b,why)||a.world!=b.world||a.local!=b.local||
    a.object==b.object||a.id==b.id){std::cerr<<"Second anchor invalid\n";return 3;}
 auto read=[&](uint32_t at,void* dst,size_t size){return rd(process.h,at,dst,size);};
 auto players=aoe2hd::ReadPlayerRoots(read,a.world);
 std::set<uint32_t> roots(players.begin(),players.end());
 roots.insert(a.world);roots.insert(a.local);
 std::ofstream vectors("aoe2hd_player_unit_vectors_v2.csv",std::ios::trunc);
 std::ofstream entries("aoe2hd_player_unit_entries_v2.csv",std::ios::trunc);
 std::ofstream diagnostic("aoe2hd_player_unit_roots_v2.csv",std::ios::trunc);
 if(!vectors||!entries||!diagnostic){std::cerr<<"CSV open failed\n";return 5;}
 vectors<<"root,offset,begin,end,stride,readable_entries,first_hits,second_hits\n";
 entries<<"phase,root,offset,begin,index,slot,field,raw,indirect,object,id,hp,direct_anchor,via_indirect,plausible\n";
 diagnostic<<"root,role,vector_candidates,readable_dwords,selection_excluded,note\n";
 struct Candidate{uint32_t root,offset,begin,end,stride;};
 std::vector<Candidate> candidateList;
 uint32_t nonWorld=0,rootWithCandidates=0;
 for(uint32_t root:roots){
  if(root<0x10000||root>UINT32_MAX-0x800)continue;
  uint32_t okWords=0;
  for(uint32_t off=0;off<0x800;off+=4){
   uint32_t word=0;if(u32(process.h,root,off,word))++okWords;
  }
  auto options=aoe2hd::DiscoverPlayerVectors(read,root,a.object,b.object);
  rootWithCandidates+=!options.empty();
  for(const auto& v:options){
   if(v.stride!=8)continue; // v2 targets exact 8-byte records
   candidateList.push_back({root,v.offset,v.begin,v.end,v.stride});
   vectors<<root<<','<<v.offset<<','<<v.begin<<','<<v.end<<','<<v.stride<<','
          <<v.readable<<','<<v.first<<','<<v.second<<"\n";
   nonWorld+=(root!=a.world);
  }
  diagnostic<<root<<','<<(root==a.world?"World":root==a.local?"LocalPlayer":"Player")
            <<','<<options.size()<<','<<okWords<<",1,"
            <<(options.empty()?(okWords?"no_plausible_vector":"root_unreadable"):"vector_hypotheses")<<"\n";
 }
 // Force known World+0x88 into the experimental inventory, even if its
 // container changes layout or does not satisfy generic vector heuristics.
 uint32_t triple[3]{};
 if(rd(process.h,a.world+0x88,triple,sizeof(triple))&&
    triple[0]>=0x10000&&triple[1]>=triple[0]&&
    (triple[1]-triple[0])%8==0&&triple[1]-triple[0]<=65536){
  bool seen=false;
  for(const auto& c:candidateList)
   if(c.root==a.world&&c.offset==0x88)seen=true;
  if(!seen)candidateList.push_back({a.world,0x88,triple[0],triple[1],8});
 }
 // Deduplicate physical entry locations across equivalent candidates,
 // while preserving candidate provenance in vector CSV.
 struct Snapshot{uint32_t root,offset,begin,slot,index,field;aoe2hd::PlayerEntryEvidence e;};
 std::vector<Snapshot> before;
 auto inspect=[&](const char* phase,std::vector<Snapshot>* collect){
  uint32_t plausibleCount=0;
  for(const auto& c:candidateList){
   const uint32_t count=(c.end-c.begin)/8;
   for(uint32_t i=0;i<count;++i){
    const uint32_t slot=c.begin+i*8;
    for(uint32_t field: {0u,4u}){
     auto e=aoe2hd::InspectPlayerEntry(read,slot,i,field,a.object,b.object);
     entries<<phase<<','<<c.root<<','<<c.offset<<','<<c.begin<<','<<i<<','
            <<slot<<','<<field<<','<<e.raw<<','<<e.indirect<<','
            <<e.object<<','<<e.id<<','<<e.hp<<','<<e.direct<<','
            <<e.viaIndirect<<','<<e.plausible<<"\n";
     plausibleCount+=e.plausible;
     if(collect)collect->push_back({c.root,c.offset,c.begin,slot,i,field,e});
    }
   }
  }
  return plausibleCount;
 };
 auto beforePlausible=inspect("before",&before);
 uint32_t idA=0,idB=0;
 if(!u32(process.h,a.object,8,idA)||idA!=a.id||
    !u32(process.h,b.object,8,idB)||idB!=b.id){
  std::cerr<<"Object identities changed\n";return 4;
 }
 std::cout<<"PlayerUnitCollectionDiscovery v2 roots="<<roots.size()
          <<" vector8="<<candidateList.size()
          <<" plausible_fields_before="<<beforePlausible<<"\n"
          <<"Create ONE villager (you may select Town Center), do NOT select the new villager, then press ENTER.\n";
 std::getline(std::cin,line);
 uint32_t ownerNow=0,worldNow=0;
 if(!u32(process.h,base,0x006F2C58,ownerNow)||ownerNow!=a.owner||
    !u32(process.h,a.owner,0xC8,worldNow)||worldNow!=a.world||
    !u32(process.h,a.object,8,idA)||idA!=a.id||
    !u32(process.h,b.object,8,idB)||idB!=b.id){
  std::cerr<<"Identity or World changed, delta unsafe\n";return 4;
 }
 auto afterPlausible=inspect("after",nullptr);
 std::ofstream delta("aoe2hd_player_unit_delta_v2.csv",std::ios::trunc);
 if(!delta){std::cerr<<"Delta CSV open failed\n";return 5;}
 delta<<"root,offset,slot,field,before_raw,after_raw,before_id,after_id,before_plausible,after_plausible,changed\n";
 uint32_t changed=0;
 for(const auto& s:before){
  auto later=aoe2hd::InspectPlayerEntry(read,s.slot,s.index,s.field,a.object,b.object);
  bool differs=s.e.raw!=later.raw||s.e.indirect!=later.indirect||
    s.e.id!=later.id||s.e.plausible!=later.plausible;
  changed+=differs;
  delta<<s.root<<','<<s.offset<<','<<s.slot<<','<<s.field<<','
       <<s.e.raw<<','<<later.raw<<','<<s.e.id<<','<<later.id<<','
       <<s.e.plausible<<','<<later.plausible<<','<<differs<<"\n";
 }
 std::cout<<"V2 plausible_fields_after="<<afterPlausible
          <<" changed_fields="<<changed<<" player_root_vectors="<<nonWorld<<"\n"
          <<"CSV: aoe2hd_player_unit_vectors_v2.csv\n"
          <<"CSV: aoe2hd_player_unit_entries_v2.csv\n"
          <<"CSV: aoe2hd_player_unit_roots_v2.csv\n"
          <<"CSV: aoe2hd_player_unit_delta_v2.csv\n"
          <<"No unit collection is confirmed.\n";
 return 0;
}

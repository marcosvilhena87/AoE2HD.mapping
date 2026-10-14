#include "UnitReferenceDiscovery.h"
#include "UnitCollectionDiscovery.h"
#include "UnitIdentityDiscovery.h"
#include "EntityGraphDiscovery.h"
#include "AdaptiveEntityGraph.h"
#include "StructuralGraphEvidence.h"
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
 const uint32_t id=pid(),base=id?module(id):0;
 if(!base){std::cerr<<"AoK HD.exe unavailable\n";return 2;}
 Handle process(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,id));
 if(!process.h){std::cerr<<"Read-only OpenProcess failed\n";return 2;}
 Anchor a,b;std::string why;
 if(!acquire(process.h,base,a,why)){std::cerr<<"First anchor: "<<why<<"\n";return 2;}
 std::cout<<"First anchor 0x"<<std::hex<<a.object<<std::dec<<" id="<<a.id
          <<"\nSelect a DIFFERENT villager and press ENTER.\n";
 std::string line;std::getline(std::cin,line);
 if(!acquire(process.h,base,b,why)||a.world!=b.world||a.local!=b.local||
    a.object==b.object||a.id==b.id){std::cerr<<"Second anchor invalid: "<<why<<"\n";return 3;}
 uint32_t firstId=0;
 if(!u32(process.h,a.object,8,firstId)||firstId!=a.id){
  std::cerr<<"First anchor changed\n";return 3;
 }
 auto reader=[&](uint32_t addr,void* dst,size_t n){return rd(process.h,addr,dst,n);};
 // World and LocalPlayer are the structural roots. The anchors are not
 // added to roots: that would falsely manufacture paths to known units.
 auto graph=aoe2hd::ExploreAdaptiveGraph(reader,a.world,a.local,a.object,b.object,512,8192);

 // Structural validation: require each candidate target to be readable.
 std::set<uint32_t> validated;
 std::set<uint32_t> targets{a.object,b.object};
 uint32_t unreadable=0;
 for(const auto& edge:graph.edges){
  if(aoe2hd::ValidateReadablePointer(reader,edge.to)){
   validated.insert(edge.to);
   if(!edge.selection)targets.insert(edge.to);
  }else ++unreadable;
 }
 // Reverse scan is bounded and focuses on readable regions containing known
 // roots, anchors or graph nodes. It never writes game memory.
 const auto mapped=enumerate(process.h,b);
 std::vector<aoe2hd::StructuralHit> reverse;
 std::vector<uint8_t> chunk(65536);
 uint64_t scanned=0,attempted=0;
 const uint64_t maxBytes=64ull*1024*1024;
 for(const auto& region:mapped){
  if(scanned>=maxBytes||reverse.size()>=4096)break;
  // Prioritize regions containing known structural nodes and both anchors.
  bool relevant=(a.object>=region.base&&uint64_t(a.object)<uint64_t(region.base)+region.size)||
    (b.object>=region.base&&uint64_t(b.object)<uint64_t(region.base)+region.size);
  if(!relevant)for(uint32_t value:targets)
   if(value>=region.base&&uint64_t(value)<uint64_t(region.base)+region.size){relevant=true;break;}
  if(!relevant)continue;
  for(uint64_t off=0;off<region.size&&scanned<maxBytes&&reverse.size()<4096;){
   size_t n=static_cast<size_t>(std::min<uint64_t>(
    {chunk.size(),uint64_t(region.size)-off,maxBytes-scanned}));
   if(n<4)break;
   SIZE_T got=0;uint32_t addr=region.base+static_cast<uint32_t>(off);
   BOOL ok=ReadProcessMemory(process.h,reinterpret_cast<LPCVOID>(uintptr_t(addr)),
                             chunk.data(),n,&got);
   attempted+=n;scanned+=n;
   if(ok&&got>=4){
    auto hits=aoe2hd::FindReverseInBuffer(addr,chunk.data(),got,targets,a.local,
                                           4096-static_cast<uint32_t>(reverse.size()));
    reverse.insert(reverse.end(),hits.begin(),hits.end());
   }
   off+=n;
  }
 }
 Anchor check;std::string why2;
 if(!acquire(process.h,base,check,why2)||check.object!=b.object||
    check.world!=a.world||check.local!=a.local||check.id!=b.id||
    !u32(process.h,a.object,8,firstId)||firstId!=a.id){
  std::cerr<<"Session changed during scan; CSV not written\n";return 4;
 }
 std::ofstream edges("aoe2hd_entity_graph_edges_v3.csv",std::ios::trunc);
 if(!edges){std::cerr<<"CSV write failed\n";return 5;}
 edges<<"pid,world,local,first_anchor,first_id,second_anchor,second_id,root,from,offset,to,depth,is_first,is_second,selection_edge,status\n";
 uint32_t firstMatches=0,secondMatches=0,selectionMatches=0,nonSelectionMatches=0;
 for(const auto& edge:graph.edges){
  firstMatches+=edge.first;secondMatches+=edge.second;
  selectionMatches+=(edge.first||edge.second)&&edge.selection;
  nonSelectionMatches+=(edge.first||edge.second)&&!edge.selection;
  edges<<id<<','<<a.world<<','<<a.local<<','<<a.object<<','<<a.id<<','
       <<b.object<<','<<b.id<<','<<edge.root<<','<<edge.from<<','
       <<edge.offset<<','<<edge.to<<','<<edge.depth<<','
       <<edge.first<<','<<edge.second<<','<<edge.selection<<",hypothesis\n";
 }
 std::ofstream reversed("aoe2hd_entity_graph_reverse_v3.csv",std::ios::trunc);
 if(!reversed){std::cerr<<"Reverse CSV failed\n";return 5;}
 reversed<<"address,target,selection_edge,target_is_first,target_is_second,status\n";
 uint32_t selectionReverse=0;
 for(const auto& hit:reverse){
  selectionReverse+=hit.selection;
  reversed<<hit.address<<','<<hit.target<<','<<hit.selection<<','
          <<(hit.target==a.object)<<','<<(hit.target==b.object)<<",hypothesis\n";
 }
 std::ofstream summary("aoe2hd_entity_graph_summary_v3.csv",std::ios::trunc);
 if(!summary){std::cerr<<"Summary write failed\n";return 5;}
 summary<<"pid,world,local,first_anchor,second_anchor,nodes,edges,first_matches,second_matches,read_failures,capped,selection_matches,nonselection_matches,validated_targets,unreadable_edges,reverse_hits,reverse_selection,bytes_examined,status\n"
        <<id<<','<<a.world<<','<<a.local<<','<<a.object<<','<<b.object<<','
        <<graph.nodes<<','<<graph.edges.size()<<','<<firstMatches<<','<<secondMatches<<','
        <<graph.readFailures<<','<<graph.capped<<','<<selectionMatches<<','<<nonSelectionMatches<<','<<validated.size()<<','<<unreadable<<','<<reverse.size()<<','<<selectionReverse<<','<<scanned<<",hypothesis\n";
 std::cout<<"EntityGraphDiscovery v3 nodes="<<graph.nodes<<" edges="<<graph.edges.size()
          <<" first_matches="<<firstMatches<<" second_matches="<<secondMatches
          <<" capped="<<graph.capped<<"\n"
          <<"CSV: aoe2hd_entity_graph_edges_v2.csv\n"
          <<"CSV: aoe2hd_entity_graph_summary_v2.csv\n"
          <<"No graph path is a confirmed unit collection.\n";
 return 0;
}

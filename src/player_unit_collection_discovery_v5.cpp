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
 if(!base){std::cerr<<"AoK HD.exe unavailable\n";return 2;}
 Handle process(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,processId));
 if(!process.h)return 2;
 Anchor a,b;std::string why,line;
 if(!acquire(process.h,base,a,why)){std::cerr<<"A: "<<why<<"\n";return 3;}
 std::cout<<"A id="<<a.id<<"\nSelect another villager B and press ENTER.\n";
 std::getline(std::cin,line);
 if(!acquire(process.h,base,b,why)||a.object==b.object||a.id==b.id||
    a.world!=b.world||a.local!=b.local){std::cerr<<"Invalid B\n";return 3;}
 auto read=[&](uint32_t p,void* dst,size_t n){return rd(process.h,p,dst,n);};
 auto players=aoe2hd::ReadPlayerRoots(read,a.world);
 struct Origin{uint32_t player,sub,field,target;};
 std::vector<Origin> origins;
 std::map<uint32_t,aoe2hd::ContentSnapshotV5> before;
 std::set<uint32_t> seenSubs;
 const uint32_t maxTargets=2048;
 uint32_t attempted=0;
 for(uint32_t player:players){
  uint32_t sub=0;
  if(!u32(process.h,player,0x14,sub)||sub<0x10000||(sub&3u)||
     sub==a.world||sub==a.object||sub==b.object||!seenSubs.insert(sub).second)continue;
  for(uint32_t off=0;off<0x400;off+=4){
   uint32_t ptr=0;
   if(!u32(process.h,sub,off,ptr)||
      !aoe2hd::UsefulOwnedPointer(ptr,a.world,a.object,b.object)||
      ptr==sub||ptr==player)continue;
   if(before.find(ptr)==before.end()){
    if(before.size()>=maxTargets)continue;
    ++attempted;
    auto snapshot=aoe2hd::SnapshotContentV5(read,ptr);
    if(!snapshot.readable)continue;
    before.emplace(ptr,std::move(snapshot));
   }
   origins.push_back({player,sub,off,ptr});
  }
 }
 std::cout<<"V5 player_subobjects="<<seenSubs.size()
          <<" readable_targets="<<before.size()<<" attempted_targets="<<attempted<<"\n"
          <<"Create ONE villager; selecting the Town Center is allowed. Do NOT select the new villager. Press ENTER when done.\n";
 std::getline(std::cin,line);
 uint32_t owner=0,world=0,idA=0,idB=0;
 if(!u32(process.h,base,0x006F2C58,owner)||owner!=a.owner||
    !u32(process.h,a.owner,0xc8,world)||world!=a.world||
    !u32(process.h,a.object,8,idA)||idA!=a.id||
    !u32(process.h,b.object,8,idB)||idB!=b.id){
  std::cerr<<"World or original objects changed; no CSV written\n";return 4;
 }
 std::ofstream delta("aoe2hd_player_indirect_delta_v5.csv");
 std::ofstream paths("aoe2hd_player_indirect_paths_v5.csv");
 std::ofstream summary("aoe2hd_player_indirect_summary_v5.csv");
 if(!delta||!paths||!summary)return 5;
 delta<<"target,readable_before,readable_after,changed_bytes,first_changed_offset,has_anchor_A,has_anchor_B,status\n";
 paths<<"player,subobject,field_offset,target,target_changed_bytes,status\n";
 std::map<uint32_t,uint32_t> changeCount;
 uint32_t changedTargets=0,unreadableAfter=0,dual=0;
 for(const auto& it:before){
  uint32_t ptr=it.first;
  auto now=aoe2hd::SnapshotContentV5(read,ptr);
  uint32_t changed=aoe2hd::ChangedBytesV5(it.second,now);
  changedTargets+=(changed>0);unreadableAfter+=!now.readable;
  int firstOffset=-1;
  uint32_t hitsA=0,hitsB=0;
  if(it.second.readable&&now.readable){
   for(size_t i=0;i<now.bytes.size();++i)
    if(it.second.bytes[i]!=now.bytes[i]){firstOffset=int(i);break;}
   for(size_t i=0;i+4<=now.bytes.size();i+=4){
    uint32_t value=0;std::memcpy(&value,now.bytes.data()+i,4);
    hitsA+=(value==a.object);hitsB+=(value==b.object);
   }
  }
  dual+=(hitsA&&hitsB);
  changeCount[ptr]=changed;
  delta<<ptr<<",1,"<<now.readable<<','<<changed<<','<<firstOffset<<','
       <<hitsA<<','<<hitsB<<",hypothesis\n";
 }
 for(const auto& o:origins)
  paths<<o.player<<','<<o.sub<<','<<o.field<<','<<o.target<<','
       <<changeCount[o.target]<<",hypothesis\n";
 summary<<"pid,world,local,A,B,player_subobjects,attempted_targets,readable_targets,changed_targets,unreadable_after,dual_anchor_targets,status\n"
        <<processId<<','<<a.world<<','<<a.local<<','<<a.object<<','<<b.object<<','
        <<seenSubs.size()<<','<<attempted<<','<<before.size()<<','
        <<changedTargets<<','<<unreadableAfter<<','<<dual<<",hypothesis\n";
 std::cout<<"V5 changed_targets="<<changedTargets
          <<" unreadable_after="<<unreadableAfter
          <<" dual_anchor_targets="<<dual<<"\n"
          <<"CSV: aoe2hd_player_indirect_delta_v5.csv\n"
          <<"CSV: aoe2hd_player_indirect_paths_v5.csv\n"
          <<"CSV: aoe2hd_player_indirect_summary_v5.csv\n"
          <<"Indirect changes do not confirm unit enumeration.\n";
 return 0;
}

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
 if(!process.h){std::cerr<<"OpenProcess failed\n";return 2;}
 Anchor a,b;std::string why,line;
 if(!acquire(process.h,base,a,why)){std::cerr<<"First anchor: "<<why<<"\n";return 3;}
 std::cout<<"A id="<<a.id<<" address=0x"<<std::hex<<a.object<<std::dec
          <<"\nSelect a DIFFERENT villager B and press ENTER.\n";
 std::getline(std::cin,line);
 if(!acquire(process.h,base,b,why)||a.object==b.object||a.id==b.id||
    a.world!=b.world||a.local!=b.local){std::cerr<<"Invalid B\n";return 3;}
 auto read=[&](uint32_t p,void* dst,size_t n){return rd(process.h,p,dst,n);};
 const auto af=aoe2hd::FingerprintObject(read,a.object),bf=aoe2hd::FingerprintObject(read,b.object);
 if(!af.readable||!bf.readable){std::cerr<<"Anchor fingerprint unreadable\n";return 4;}
 std::ofstream objects("aoe2hd_player_objects_v3.csv");
 std::ofstream rootsFile("aoe2hd_player_subobjects_v3.csv");
 std::ofstream summary("aoe2hd_player_summary_v3.csv");
 if(!objects||!rootsFile||!summary)return 5;
 objects<<"source,entry,field,raw,object,vtable,id,owner,hp,hp_sane,classification\n";
 rootsFile<<"player,offset,subobject,subobject_offset,begin,end,stride,A_hits,B_hits,status\n";
 // World+0x88: interpret each 4-byte field as one pointer, NOT
 // automatically as two unrelated 8-byte object records.
 uint32_t start=0,end=0,capacity=0;
 if(!u32(process.h,a.world,0x88,start)||!u32(process.h,a.world,0x8c,end)||
    !u32(process.h,a.world,0x90,capacity)||start<0x10000||end<start||
    capacity<end||end-start>65536||(end-start)%4){
  std::cerr<<"World+0x88 not a bounded pointer vector\n";return 4;
 }
 uint32_t seenA=0,seenB=0,vtMatch=0,readable=0;
 for(uint32_t at=start;at<end;at+=4){
  uint32_t value=0;if(!rd(process.h,at,&value,4))continue;
  auto o=aoe2hd::FingerprintObject(read,value);
  readable+=o.readable;seenA+=(value==a.object);seenB+=(value==b.object);
  const char* type=aoe2hd::ClassifyObject(o,af,bf);
  vtMatch+=(std::string(type)=="anchor_vtable_match"||
            std::string(type)=="A_vtable_match"||
            std::string(type)=="B_vtable_match");
  objects<<"World+0x88"<<','<<(at-start)/4<<','<<0<<','<<value<<','
         <<o.address<<','<<o.vt<<','<<o.id<<','<<o.owner<<','
         <<o.hp<<','<<o.hpSane<<','<<type<<"\n";
 }
 // One extra level of player-origin discovery. No expansion through
 // the selection slots and no graph traversal of arbitrary game memory.
 auto players=aoe2hd::ReadPlayerRoots(read,a.world);
 std::set<uint32_t> uniquePlayers(players.begin(),players.end());
 uint32_t subobjects=0,dual=0;
 for(uint32_t player:uniquePlayers){
  if(player<0x10000||player>UINT32_MAX-0x800)continue;
  std::set<uint32_t> seen;
  for(uint32_t off=0;off<0x800;off+=4){
   if(player==a.local&&(off==0x15c||off==0x160))continue;
   uint32_t sub=0;
   if(!u32(process.h,player,off,sub)||sub<0x10000||(sub&3)||sub>UINT32_MAX-0x400)continue;
   if(!seen.insert(sub).second)continue;
   // Avoid treating the known selected objects as collection roots.
   if(sub==a.object||sub==b.object)continue;
   auto candidates=aoe2hd::DiscoverPlayerVectors(read,sub,a.object,b.object,0x400);
   for(const auto& v:candidates){
    ++subobjects;dual+=v.first&&v.second;
    rootsFile<<player<<','<<off<<','<<sub<<','<<v.offset<<','<<v.begin<<','
             <<v.end<<','<<v.stride<<','<<v.first<<','<<v.second
             <<",hypothesis\n";
   }
  }
 }
 uint32_t idA=0,idB=0,world=0;
 if(!u32(process.h,a.object,8,idA)||idA!=a.id||
    !u32(process.h,b.object,8,idB)||idB!=b.id||
    !u32(process.h,a.owner,0xc8,world)||world!=a.world){
  std::cerr<<"Session identity changed; discard CSV\n";return 4;
 }
 summary<<"pid,world,local,A,B,world88_begin,world88_end,world88_entries,readable_objects,matched_vtable,anchor_A_in_world88,anchor_B_in_world88,player_roots,subobject_vector_interpretations,dual_anchor_candidates,status\n";
 summary<<processId<<','<<a.world<<','<<a.local<<','<<a.object<<','<<b.object<<','
        <<start<<','<<end<<','<<(end-start)/4<<','<<readable<<','<<vtMatch<<','
        <<seenA<<','<<seenB<<','<<uniquePlayers.size()<<','<<subobjects<<','<<dual
        <<",hypothesis\n";
 std::cout<<"PlayerUnitCollectionDiscovery v3 World+0x88 entries="<<(end-start)/4
          <<" readable="<<readable<<" vtable_matches="<<vtMatch
          <<" anchors="<<seenA<<"/"<<seenB
          <<" subobject_vectors="<<subobjects<<" dual="<<dual<<"\n"
          <<"CSV: aoe2hd_player_objects_v3.csv\n"
          <<"CSV: aoe2hd_player_subobjects_v3.csv\n"
          <<"CSV: aoe2hd_player_summary_v3.csv\n"
          <<"No unit collection confirmed.\n";
 return 0;
}

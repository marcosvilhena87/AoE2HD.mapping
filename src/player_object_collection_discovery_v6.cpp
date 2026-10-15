#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <map>
#include <sstream>
#include "BuildingReferenceDiscoveryV1.h"
#include "BuildingObjectDiscoveryV2.h"
#include "ProductionQueueReader.h"
#include "BuildingObjectDiscoveryV4.h"
#include "BuildingObjectDiscoveryV5.h"
#include "BuildingObjectDiscoveryV6.h"
#include "BuildingObjectDiscoveryV7.h"
#include "PlayerObjectCollectionDiscoveryV1.h"
#include "PlayerObjectCollectionDiscoveryV2.h"
#include "PlayerObjectCollectionDiscoveryV3.h"
#include "PlayerObjectCollectionDiscoveryV4.h"
#include "PlayerObjectCollectionDiscoveryV5.h"
#include "PlayerObjectCollectionDiscoveryV6.h"
namespace {
struct Handle{HANDLE value;explicit Handle(HANDLE v):value(v){}~Handle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);}};
bool rd(HANDLE h,uint32_t addr,void* dst,size_t len){SIZE_T got=0;return ReadProcessMemory(h,reinterpret_cast<LPCVOID>(uintptr_t(addr)),dst,len,&got)&&got==len;}
bool w(HANDLE h,uint32_t addr,uint32_t off,uint32_t& out){return addr<=UINT32_MAX-off&&rd(h,addr+off,&out,4);}
uint32_t pid(){Handle h(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));if(h.value==INVALID_HANDLE_VALUE)return 0;PROCESSENTRY32W p{};p.dwSize=sizeof(p);if(!Process32FirstW(h.value,&p))return 0;do{if(_wcsicmp(p.szExeFile,L"AoK HD.exe")==0)return p.th32ProcessID;}while(Process32NextW(h.value,&p));return 0;}
uint32_t module(uint32_t id){Handle h(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,id));if(h.value==INVALID_HANDLE_VALUE)return 0;MODULEENTRY32W m{};m.dwSize=sizeof(m);if(!Module32FirstW(h.value,&m))return 0;do{if(_wcsicmp(m.szModule,L"AoK HD.exe")==0)return uint32_t(uintptr_t(m.modBaseAddr));}while(Module32NextW(h.value,&m));return 0;}
bool readable(const MEMORY_BASIC_INFORMATION& m){
 DWORD p=m.Protect&0xff;
 return m.State==MEM_COMMIT&&!(m.Protect&PAGE_GUARD)&&!(m.Protect&PAGE_NOACCESS)&&
 (p==PAGE_READONLY||p==PAGE_READWRITE||p==PAGE_WRITECOPY||p==PAGE_EXECUTE_READ||p==PAGE_EXECUTE_READWRITE||p==PAGE_EXECUTE_WRITECOPY);
}
}
int main(){
 uint32_t p=pid(),base=p?module(p):0;if(!base)return 2;
 Handle proc(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,p));
 if(!proc.value)return 2;
 uint32_t owner=0,world=0,vt=0,idx=0,begin=0,end=0,local=0;
 if(!w(proc.value,base,0x006F2C58,owner)||!owner||!w(proc.value,owner,0xc8,world)||
 !w(proc.value,world,0,vt)||vt!=base+0x005D329C||
 !w(proc.value,world,0x174,idx)||!w(proc.value,world,0x184,begin)||
 !w(proc.value,world,0x188,end)||end<begin||(end-begin)%8||idx==0||
 idx>=(end-begin)/8||!w(proc.value,begin,idx*8,local))return 3;
 auto selected=[&](uint32_t& addr,uint32_t& id){
  uint32_t a=0,b=0;if(!w(proc.value,local,0x15c,a)||!w(proc.value,local,0x160,b)||
   !a||a!=b||!w(proc.value,a,8,id))return false;addr=a;return true;
 };
 std::string line;std::vector<aoe2hd::ObjectAnchorV1> known;
 const char* names[]={"Centro Urbano A","Quartel B","Centro Urbano D","Campo de Arqueiros E","Aldeao C"};
 for(const char* name:names){
  std::cout<<"Select "<<name<<" then ENTER.\n";
  if(!std::getline(std::cin,line))return 4;
  uint32_t a=0,id=0;if(!selected(a,id))return 4;
  for(const auto& k:known)if(k.address==a||k.id==id){std::cerr<<"Duplicate anchor\n";return 4;}
  std::cout<<" id="<<id<<" ptr=0x"<<std::hex<<a<<std::dec<<" confirm y/n: ";
  if(!std::getline(std::cin,line)||(line!="y"&&line!="Y"))return 4;
  known.push_back({a,id});
 }
 std::cout<<"Select a different, unregistered object and ENTER to begin independent snapshots.\n";
 if(!std::getline(std::cin,line))return 4;
 uint32_t other=0,otherId=0;if(!selected(other,otherId))return 4;
 for(const auto& k:known)if(k.address==other)return 4;
 std::vector<uint32_t> roots={world,local};
 auto probe=[&](uint32_t addr,size_t length)->bool{
  if(!addr||uint64_t(addr)+length>0x100000000ull)return false;
  uintptr_t pos=addr,finish=uintptr_t(addr)+length;
  while(pos<finish){
   MEMORY_BASIC_INFORMATION m{};
   if(!VirtualQueryEx(proc.value,reinterpret_cast<LPCVOID>(pos),&m,sizeof(m))||!readable(m))return false;
   uintptr_t regionEnd=uintptr_t(m.BaseAddress)+m.RegionSize;
   if(regionEnd<=pos)return false;
   pos=std::min(regionEnd,finish);
  }
  return true;
 };


 std::cout<<"Candidate addresses (hex), separated by whitespace; empty = use default v5 shortlist:\n";
 std::cout<<" 0x14767530 0x188DBE38 0x14D6B188 0x14935850 0x14A4B020\n";
 if(!std::getline(std::cin,line))return 4;
 std::istringstream entries(line.empty()?"14767530 188DBE38 14D6B188 14935850 14A4B020":line);
 std::vector<uint32_t> targets;std::string token;
 while(entries>>token&&targets.size()<16){
  try {size_t at=0;unsigned long v=std::stoul(token,&at,16);
   if(at==token.size()&&v>=0x10000&&v<=UINT32_MAX-1024&&!(v&3u))
    if(std::find(targets.begin(),targets.end(),uint32_t(v))==targets.end())targets.push_back(uint32_t(v));
  }catch(...){}
 }
 if(targets.empty())return 4;
 struct Sample {uint32_t address=0;bool readable=false;aoe2hd::CandidateEvidenceV6 metrics;};
 auto capture=[&](){
  std::vector<Sample> out;
  for(auto address:targets){
   Sample sample;sample.address=address;
   std::vector<uint8_t> bytes(1024);
   sample.readable=probe(address,bytes.size())&&rd(proc.value,address,bytes.data(),bytes.size());
   if(sample.readable)sample.metrics=aoe2hd::InspectTargetV6(bytes,known,
    [&](uint32_t obj,uint32_t& id){return probe(obj,12)&&w(proc.value,obj,8,id);});
   out.push_back(sample);
  }
  return out;
 };
 std::cout<<"Snapshot 1: choose a non-anchor object, then ENTER.\n";
 if(!std::getline(std::cin,line))return 4;
 auto first=capture();
 std::cout<<"Snapshot 2: change selection, then ENTER.\n";
 if(!std::getline(std::cin,line))return 4;
 auto second=capture();
 std::ofstream csv("aoe2hd_player_object_collection_targeted_v6.csv");
 if(!csv)return 5;
 csv<<"address,readable_first,readable_second,content_equal,pointer_slots,unique_targets,readable_ids,plausible_ids,longest_run,vector_triples,anchor_matches,distinct_anchors,score\n";
 size_t readable=0,unchanged=0,anchorsFound=0;
 for(size_t i=0;i<targets.size();++i){
  const auto& a=first[i];const auto& b=second[i];
  bool equal=a.readable&&b.readable&&a.metrics.fingerprint==b.metrics.fingerprint;
  readable+=b.readable;unchanged+=equal;anchorsFound+=b.metrics.anchorMatches;
  csv<<b.address<<','<<a.readable<<','<<b.readable<<','<<equal<<','
     <<b.metrics.pointerSlots<<','<<b.metrics.uniqueTargets<<','
     <<b.metrics.readableIds<<','<<b.metrics.plausibleIds<<','
     <<b.metrics.longestRun<<','<<b.metrics.vectorTriples<<','
     <<b.metrics.anchorMatches<<','<<b.metrics.distinctAnchors<<','
     <<b.metrics.score<<"\n";
 }
 std::cout<<"PlayerObjectCollectionDiscovery v6 candidates="<<targets.size()
          <<" readable_second="<<readable<<" unchanged_content="<<unchanged
          <<" anchor_hits="<<anchorsFound<<"\n"
          <<"CSV: aoe2hd_player_object_collection_targeted_v6.csv\n"
          <<"Targeted absolute addresses expire when the game process changes.\n"
          <<"Plausible IDs and vector patterns are not confirmed entities.\n";
 return 0;
}

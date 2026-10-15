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

 auto capture=[&](){return aoe2hd::TraverseV4(std::vector<uint32_t>{world,local},known,probe,
  [&](uint32_t address,void* dst,size_t n){return rd(proc.value,address,dst,n);});};
 std::cout<<"Snapshot 1: select unregistered object and press ENTER.\n";
 if(!std::getline(std::cin,line))return 4;
 auto first=capture();
 std::cout<<"Snapshot 2: select a different object and press ENTER.\n";
 if(!std::getline(std::cin,line))return 4;
 auto second=capture();
 auto metrics=[&](const aoe2hd::ResultV4& result){
  std::map<uint64_t,aoe2hd::CollectionMetricsV5> out;
  for(const auto& node:result.nodes){
   if(node.status!="ok")continue;
   std::vector<uint8_t> bytes(1024);
   if(!rd(proc.value,node.node.address,bytes.data(),bytes.size()))continue;
   auto m=aoe2hd::ExamineCollectionV5(bytes,known,
    [&](uint32_t a){return probe(a,16);},
    [&](uint32_t a,uint32_t& id){return w(proc.value,a,8,id);});
   uint64_t key=(uint64_t(node.node.root)<<32)|node.node.address;
   out[key]=m;
  }
  return out;
 };
 // Both metric passes read the same live process after snapshot 2; interpret
 // cross-snapshot persistence only through the v4 traversal node identities.
 auto measured=metrics(second);
 std::set<uint64_t> previous;
 for(const auto& n:first.nodes)if(n.status=="ok")
  previous.insert((uint64_t(n.node.root)<<32)|n.node.address);
 std::ofstream out("aoe2hd_player_object_collection_candidates_v5.csv");
 std::ofstream coverage("aoe2hd_player_object_collection_coverage_v5.csv");
 if(!out||!coverage)return 5;
 out<<"root,depth,address,path,persisted_node,pointer_run,valid_pointer_slots,anchor_matches,distinct_anchors,begin_end_pairs,vector_triples,plausible_object_ids,score\n";
 coverage<<"snapshot,reads,bytes_attempted,preflight_checks,truncated,anchor_hits\n";
 const char* labels[]={"World","LocalPlayer"};
 for(const auto& n:second.nodes){
  if(n.status!="ok")continue;
  uint64_t key=(uint64_t(n.node.root)<<32)|n.node.address;
  auto it=measured.find(key);if(it==measured.end())continue;
  const auto& m=it->second;
  out<<labels[n.node.root]<<','<<n.node.depth<<','<<n.node.address<<",\""<<n.node.path<<"\","
    <<previous.count(key)<<','<<m.longestPointerRun<<','<<m.validPointerSlots<<','
    <<m.anchorMatches<<','<<m.distinctAnchors<<','<<m.beginEndPairs<<','
    <<m.beginEndCapacityTriples<<','<<m.validatedObjectIDs<<','<<m.score<<"\n";
 }
 coverage<<"first,"<<first.reads<<','<<first.bytesAttempted<<','<<first.preflightCalls<<','
         <<first.truncated<<','<<first.hits.size()<<"\n";
 coverage<<"second,"<<second.reads<<','<<second.bytesAttempted<<','<<second.preflightCalls<<','
         <<second.truncated<<','<<second.hits.size()<<"\n";
 std::cout<<"PlayerObjectCollectionDiscovery v5 candidates="<<measured.size()
          <<" persisted_nodes="<<previous.size()<<" anchor_hits="<<second.hits.size()
          <<" truncated="<<second.truncated<<"\n"
          <<"CSV: aoe2hd_player_object_collection_candidates_v5.csv\n"
          <<"CSV: aoe2hd_player_object_collection_coverage_v5.csv\n"
          <<"Collection scores are exploratory, not confirmed object enumeration.\n";
 return 0;
}

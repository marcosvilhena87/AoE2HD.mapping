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
 auto result=aoe2hd::TraverseObjectsV3(roots,known,
  [&](uint32_t address,void* dst,size_t n){return rd(proc.value,address,dst,n);});
 std::ofstream paths("aoe2hd_player_object_collection_paths_v3.csv");
 std::ofstream nodes("aoe2hd_player_object_collection_nodes_v3.csv");
 std::ofstream coverage("aoe2hd_player_object_collection_coverage_v3.csv");
 if(!paths||!nodes||!coverage)return 5;
 paths<<"root,depth,reference_slot,target_object,anchor_index,anchor_id,path\n";
 nodes<<"root,depth,address,status,discovered,anchor_hits,path\n";
 coverage<<"root,depth,attempted,successful,read_failures,duplicates,invalid,queued,cap_dropped,anchor_hits\n";
 std::set<size_t> distinct;
 const char* namesRoot[]={"World","LocalPlayer"};
 for(const auto& hit:result.hits){
  distinct.insert(hit.anchor);
  paths<<namesRoot[hit.root]<<','<<hit.depth<<','<<hit.slot<<','<<hit.target<<','
       <<hit.anchor<<','<<known[hit.anchor].id<<",\""<<hit.path<<"\"\n";
 }
 for(const auto& e:result.events)
  nodes<<namesRoot[e.root]<<','<<e.depth<<','<<e.address<<','<<e.status<<','
       <<e.discovered<<','<<e.hits<<",\""<<e.path<<"\"\n";
 for(size_t root=0;root<2;++root)for(size_t depth=0;depth<=3;++depth){
  const auto& b=result.buckets[root*4+depth];
  coverage<<namesRoot[root]<<','<<depth<<','<<b.attempted<<','<<b.successful<<','
          <<b.readFailures<<','<<b.duplicates<<','<<b.invalid<<','<<b.queued<<','
          <<b.capDropped<<','<<b.hitCount<<"\n";
 }
 std::cout<<"PlayerObjectCollectionDiscovery v3 reads="<<result.reads
          <<" bytes_attempted="<<result.bytesAttempted<<" truncated="<<result.truncated
          <<" anchor_hits="<<result.hits.size()<<" distinct_anchors="<<distinct.size()<<"\n"
          <<"CSV: aoe2hd_player_object_collection_paths_v3.csv\n"
          <<"CSV: aoe2hd_player_object_collection_nodes_v3.csv\n"
          <<"CSV: aoe2hd_player_object_collection_coverage_v3.csv\n"
          <<"Read-only candidate paths do not prove an object collection.\n";
 return 0;
}

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
#include "PlayerObjectCollectionDiscoveryV7.h"
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


 std::cout<<"Candidate addresses in hexadecimal (space separated, required; do not reuse after process restart):\n";
 if(!std::getline(std::cin,line))return 4;
 std::istringstream input(line);std::string word;std::vector<uint32_t> addresses;
 while(input>>word&&addresses.size()<16){
  try{size_t pos=0;unsigned long a=std::stoul(word,&pos,16);
   if(pos==word.size()&&a>=0x10000&&a<=UINT32_MAX-1024&&!(a&3u)&&
    std::find(addresses.begin(),addresses.end(),uint32_t(a))==addresses.end())
     addresses.push_back(uint32_t(a));
  }catch(...){}
 }
 if(addresses.empty()){std::cerr<<"No valid current-session candidates.\n";return 4;}
 struct Capture {std::vector<uint8_t> bytes;bool valid=false;};
 auto snapshot=[&](){
  std::vector<Capture> out;
  for(uint32_t address:addresses){
   Capture c;c.bytes.resize(1024);
   c.valid=probe(address,1024)&&rd(proc.value,address,c.bytes.data(),1024);
   out.push_back(std::move(c));
  }
  return out;
 };
 std::cout<<"A: record initial villager count, do not create units, then ENTER.\n";
 if(!std::getline(std::cin,line))return 4;
 auto a=snapshot();
 std::cout<<"CONTROL: do not create/destroy any object; let simulation run, then ENTER.\n";
 if(!std::getline(std::cin,line))return 4;
 auto b=snapshot();
 std::cout<<"INTERVENTION: create exactly ONE villager, wait until it appears, DO NOT select it, then ENTER.\n";
 if(!std::getline(std::cin,line))return 4;
 auto c=snapshot();
 std::ofstream out("aoe2hd_player_object_collection_differential_v7.csv");
 std::ofstream summary("aoe2hd_player_object_collection_summary_v7.csv");
 if(!out||!summary)return 5;
 out<<"address,offset,before,control,after,control_stable,increment_one,new_pointer,after_pointer_readable,after_id,after_id_matches_anchor\n";
 summary<<"address,readable_a,readable_control,readable_after,changed_control,changed_intervention,stable_increment_one,new_readable_pointers\n";
 size_t total=0,plusOne=0;
 for(size_t i=0;i<addresses.size();++i){
  size_t controlChanges=0,interventionChanges=0,increments=0,newPointers=0;
  if(a[i].valid&&b[i].valid&&c[i].valid){
   for(const auto& change:aoe2hd::CompareV7(a[i].bytes,b[i].bytes,c[i].bytes)){
    uint32_t id=0;bool targetRead=false,anchorMatch=false;
    if(change.newPointer&&probe(change.after,12)&&w(proc.value,change.after,8,id)){
     targetRead=true;++newPointers;
     for(const auto& k:known)if(k.id==id&&k.address==change.after)anchorMatch=true;
    }
    controlChanges+=!change.controlStable;
    interventionChanges+=change.control!=change.after;
    increments+=change.incrementOne;
    out<<addresses[i]<<','<<change.offset<<','<<change.before<<','<<change.control<<','<<change.after<<','
       <<change.controlStable<<','<<change.incrementOne<<','<<change.newPointer<<','
       <<targetRead<<','<<id<<','<<anchorMatch<<"\n";
   }
  }
  total+=interventionChanges;plusOne+=increments;
  summary<<addresses[i]<<','<<a[i].valid<<','<<b[i].valid<<','<<c[i].valid<<','
         <<controlChanges<<','<<interventionChanges<<','<<increments<<','<<newPointers<<"\n";
 }
 std::cout<<"PlayerObjectCollectionDiscovery v7 candidates="<<addresses.size()
          <<" intervention_changed_fields="<<total<<" stable_plus_one_fields="<<plusOne<<"\n"
          <<"CSV: aoe2hd_player_object_collection_differential_v7.csv\n"
          <<"CSV: aoe2hd_player_object_collection_summary_v7.csv\n"
          <<"Interpret changes as hypotheses; gameplay counters are not proven collections.\n";
 return 0;
}

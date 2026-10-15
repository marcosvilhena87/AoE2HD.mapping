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
#include "PlayerObjectCollectionDiscoveryV8.h"
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


 std::cout<<"Enter candidate base address in hex (current game process only):\n";
 if(!std::getline(std::cin,line))return 4;
 uint32_t candidate=0;
 try{
  size_t consumed=0;unsigned long x=std::stoul(line,&consumed,16);
  if(consumed!=line.size()||x<0x10000||x>UINT32_MAX-0x290||(x&3u))return 4;
  candidate=uint32_t(x);
 }catch(...){return 4;}
 const uint32_t offsets[]={0x178,0x28c};
 auto snapshot=[&](){
  std::vector<aoe2hd::PointerSnapshotV8> result;
  for(uint32_t off:offsets){
   aoe2hd::PointerSnapshotV8 sample;
   uint32_t slot=candidate+off;
   sample.slotRead=probe(slot,4)&&rd(proc.value,slot,&sample.pointer,4);
   if(sample.slotRead&&sample.pointer>=0x10000&&!(sample.pointer&3u)&&probe(sample.pointer,12)){
    uint32_t values[3]={};
    sample.targetRead=rd(proc.value,sample.pointer,values,sizeof(values));
    if(sample.targetRead){
     sample.firstWord=values[0];sample.id=values[2];sample.thirdWord=values[2];
    }
   }
   result.push_back(sample);
  }
  return result;
 };
 std::cout<<"A: BEFORE creation, keep unit count unchanged and press ENTER.\n";
 if(!std::getline(std::cin,line))return 4;
 auto a=snapshot();
 std::cout<<"CONTROL: wait without producing/destroying anything and press ENTER.\n";
 if(!std::getline(std::cin,line))return 4;
 auto b=snapshot();
 std::cout<<"INTERVENTION: finish creating ONE villager; do not select it; press ENTER.\n";
 if(!std::getline(std::cin,line))return 4;
 auto c=snapshot();
 std::vector<uint32_t> ids;
 for(const auto& k:known)ids.push_back(k.id);
 std::ofstream out("aoe2hd_player_object_collection_transitions_v8.csv");
 if(!out)return 5;
 out<<"base,offset,a_slot_ok,a_ptr,a_target_ok,a_id,control_slot_ok,control_ptr,control_target_ok,control_id,after_slot_ok,after_ptr,after_target_ok,after_id,control_stable,intervention_changed,after_id_matches_known,candidate\n";
 size_t hits=0;
 for(size_t i=0;i<2;++i){
  auto r=aoe2hd::ClassifyPointerV8(a[i],b[i],c[i],ids);
  hits+=r.candidate;
  out<<candidate<<','<<offsets[i]<<','<<a[i].slotRead<<','<<a[i].pointer<<','
     <<a[i].targetRead<<','<<a[i].id<<','<<b[i].slotRead<<','<<b[i].pointer<<','
     <<b[i].targetRead<<','<<b[i].id<<','<<c[i].slotRead<<','<<c[i].pointer<<','
     <<c[i].targetRead<<','<<c[i].id<<','<<r.controlStable<<','
     <<r.interventionChanged<<','<<r.idEqualsKnown<<','<<r.candidate<<"\n";
 }
 std::cout<<"PlayerObjectCollectionDiscovery v8 monitored_slots=2 candidates="<<hits<<"\n"
          <<"CSV: aoe2hd_player_object_collection_transitions_v8.csv\n"
          <<"Candidate means a readable pointer transition, not a confirmed villager.\n";
 return 0;
}

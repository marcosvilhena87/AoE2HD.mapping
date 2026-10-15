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
 uint32_t p=pid(),base=p?module(p):0;
 if(!base){std::cerr<<"AoK HD.exe unavailable\n";return 2;}
 Handle proc(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,p));
 if(!proc.value)return 2;
 uint32_t owner=0,world=0,vt=0,idx=0,begin=0,end=0,local=0;
 if(!w(proc.value,base,0x006F2C58,owner)||!owner||!w(proc.value,owner,0xc8,world)||
 !w(proc.value,world,0,vt)||vt!=base+0x005D329C||
 !w(proc.value,world,0x174,idx)||!w(proc.value,world,0x184,begin)||
 !w(proc.value,world,0x188,end)||end<begin||(end-begin)%8||idx==0||
 idx>=(end-begin)/8||!w(proc.value,begin,idx*8,local))return 3;
 auto selected=[&](uint32_t& addr,uint32_t& id){
  uint32_t a=0,b=0;
  if(!w(proc.value,local,0x15c,a)||!w(proc.value,local,0x160,b)||!a||a!=b||!w(proc.value,a,8,id))return false;
  addr=a;return true;
 };
 const char* labels[]={"Centro Urbano A","Quartel B","Centro Urbano D","Campo de Arqueiros E"};
 std::vector<aoe2hd::ObjectIdentityV7> known;
 std::string line;
 for(const char* label:labels){
  std::cout<<"Select "<<label<<", ENTER to register identity.\n";
  if(!std::getline(std::cin,line))return 4;
  uint32_t addr=0,id=0;
  if(!selected(addr,id))return 4;
  for(const auto& x:known)if(x.address==addr||x.id==id){std::cerr<<"Duplicate anchor\n";return 4;}
  std::cout<<" "<<label<<" id="<<id<<" ptr=0x"<<std::hex<<addr<<std::dec<<" confirm y/n: ";
  if(!std::getline(std::cin,line)||(line!="y"&&line!="Y"))return 4;
  known.push_back({addr,id});
 }
 uint32_t A=known[0].address;
 std::cout<<"Select Centro Urbano A again, ENTER.\n";
 uint32_t current=0,currentId=0;
 if(!std::getline(std::cin,line)||!selected(current,currentId)||current!=A)return 4;
 std::cout<<"Reference address in hex (0 = scan direct references to A): ";
 if(!std::getline(std::cin,line))return 4;
 uint32_t ref=0;
 try{unsigned long x=std::stoul(line.empty()?"0":line,nullptr,16);if(x>UINT32_MAX)return 4;ref=uint32_t(x);}
 catch(...){return 4;}
 if(!ref){
  constexpr size_t budget=96u*1024u*1024u,chunk=64u*1024u,cap=4u*1024u*1024u;
  size_t scanned=0;uintptr_t cursor=0x10000;std::vector<uint8_t> data(chunk);
  std::vector<uint32_t> refs;
  while(cursor<0x7fff0000&&scanned<budget){
   MEMORY_BASIC_INFORMATION m{};
   if(!VirtualQueryEx(proc.value,reinterpret_cast<LPCVOID>(cursor),&m,sizeof(m)))break;
   uintptr_t start=uintptr_t(m.BaseAddress),finish=start+m.RegionSize;
   if(finish<=cursor)break;
   if(readable(m)&&start<0x80000000){
    size_t take=std::min({size_t(finish-start),cap,budget-scanned});
    for(size_t off=0;off<take;off+=chunk){
     size_t n=std::min(chunk,take-off),at=start+off;
     if(at>UINT32_MAX)break;
     if(rd(proc.value,uint32_t(at),data.data(),n)){
      for(size_t j=0;j+4<=n;j+=4){
       uint32_t val=0;std::memcpy(&val,data.data()+j,4);
       if(val==A)refs.push_back(uint32_t(at+j));
      }
     }
    }
    scanned+=take;
   }
   cursor=finish;
  }
  std::cout<<"Reference candidates="<<refs.size()<<"\n";
  for(uint32_t x:refs)std::cout<<" 0x"<<std::hex<<x<<std::dec<<"\n";
  std::cout<<"Select reference hex: ";
  if(!std::getline(std::cin,line))return 4;
  try{unsigned long x=std::stoul(line,nullptr,16);if(x>UINT32_MAX)return 4;ref=uint32_t(x);}
  catch(...){return 4;}
  if(std::find(refs.begin(),refs.end(),ref)==refs.end())return 4;
 }
 if(ref<0x10800||ref>UINT32_MAX-2048||(ref&3u))return 4;
 uint32_t window=ref-2048;
 std::ofstream hits("aoe2hd_building_object_identity_hits_v7.csv");
 std::ofstream state("aoe2hd_building_object_identity_stages_v7.csv");
 if(!hits||!state)return 5;
 hits<<"stage,slot,value,matched_label,matched_object_id\n";
 state<<"stage,selected_addr,selected_id,reference_value,reference_points_to_A,window_readable,matching_slots,distinct_known_objects\n";
 const char* stages[]={"A","B","C","D","E","A_again"};
 const char* instructions[]={"Centro Urbano A","Quartel B","Aldeao C","Centro Urbano D","Campo de Arqueiros E","Centro Urbano A"};
 for(size_t phase=0;phase<6;++phase){
  std::cout<<"STAGE "<<stages[phase]<<" select "<<instructions[phase]<<", ENTER; q quit.\n";
  if(!std::getline(std::cin,line))return 4;if(line=="q")break;
  uint32_t now=0,nowId=0,o=0,wo=0;
  if(!selected(now,nowId)||!w(proc.value,base,0x006F2C58,o)||o!=owner||
   !w(proc.value,owner,0xc8,wo)||wo!=world){std::cerr<<"Session changed\n";return 4;}
  for(const auto& object:known){uint32_t id=0;if(!w(proc.value,object.address,8,id)||id!=object.id){std::cerr<<"Anchor changed\n";return 4;}}
  uint32_t referenceValue=0;bool ok=rd(proc.value,ref,&referenceValue,4);
  std::vector<uint8_t> data(4096);bool valid=rd(proc.value,window,data.data(),data.size());
  std::vector<aoe2hd::IdentityHitV7> found;
  if(valid)found=aoe2hd::MatchKnownObjectsV7(window,data,known);
  std::set<int> distinct;
  for(auto x:found){distinct.insert(x.matched);
   hits<<stages[phase]<<','<<x.slot<<','<<x.value<<','<<labels[x.matched]<<','<<known[x.matched].id<<"\n";
  }
  state<<stages[phase]<<','<<now<<','<<nowId<<','<<(ok?referenceValue:0)<<','
       <<(ok&&referenceValue==A)<<','<<valid<<','<<found.size()<<','<<distinct.size()<<"\n";
  state.flush();hits.flush();
  std::cout<<" matching_slots="<<found.size()<<" distinct_known_objects="<<distinct.size()<<"\n";
 }
 std::cout<<"CSV: aoe2hd_building_object_identity_hits_v7.csv\n"
          <<"CSV: aoe2hd_building_object_identity_stages_v7.csv\n"
          <<"Exact anchors establish direct matches only, not autonomous discovery.\n";
 return 0;
}

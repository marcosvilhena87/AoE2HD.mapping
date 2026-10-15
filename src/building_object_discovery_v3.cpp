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
 uint32_t processId=pid(),base=processId?module(processId):0;
 if(!base){std::cerr<<"AoK HD.exe unavailable\n";return 2;}
 Handle process(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,processId));
 if(!process.value)return 2;
 uint32_t owner=0,world=0,vt=0,index=0,begin=0,end=0,local=0;
 if(!w(process.value,base,0x006F2C58,owner)||!owner||!w(process.value,owner,0xc8,world)||
    !w(process.value,world,0,vt)||vt!=base+0x005D329C||
    !w(process.value,world,0x174,index)||!w(process.value,world,0x184,begin)||
    !w(process.value,world,0x188,end)||end<begin||(end-begin)%8||
    index==0||index>=(end-begin)/8||!w(process.value,begin,index*8,local))return 3;
 auto selected=[&](uint32_t& ptr,uint32_t& id){
  uint32_t x=0,y=0;
  if(!w(process.value,local,0x15c,x)||!w(process.value,local,0x160,y)||!x||x!=y||
     !w(process.value,x,8,id))return false;
  ptr=x;return true;
 };
 std::string line;uint32_t anchor=0,identity=0;
 std::cout<<"Select known building A, press ENTER.\n";
 if(!std::getline(std::cin,line)||!selected(anchor,identity))return 4;
 std::cout<<"A id="<<identity<<" ptr=0x"<<std::hex<<anchor<<std::dec<<" confirm (y/n): ";
 if(!std::getline(std::cin,line)||(line!="y"&&line!="Y"))return 4;
 std::cout<<"Enter reference address in hex from v2 (0 = bounded discovery scan): ";
 if(!std::getline(std::cin,line))return 4;
 uint32_t reference=0;
 try {
  unsigned long parsed=std::stoul(line.empty()?"0":line,nullptr,16);
  if(parsed>UINT32_MAX)return 4;reference=uint32_t(parsed);
 }catch(...){std::cerr<<"Invalid address\n";return 4;}
 std::vector<uint32_t> refs;
 if(reference){refs.push_back(reference);}
 else {
  constexpr size_t budget=96u*1024u*1024u,chunk=64u*1024u,regionCap=4u*1024u*1024u;
  size_t scanned=0;uintptr_t addr=0x10000;std::vector<uint8_t> data(chunk);
  while(addr<0x7fff0000&&scanned<budget){
   MEMORY_BASIC_INFORMATION m{};
   if(!VirtualQueryEx(process.value,reinterpret_cast<LPCVOID>(addr),&m,sizeof(m)))break;
   uintptr_t start=uintptr_t(m.BaseAddress),end=start+m.RegionSize;
   if(end<=addr)break;
   if(readable(m)&&start<0x80000000){
    size_t take=std::min({size_t(end-start),regionCap,budget-scanned});
    for(size_t off=0;off<take;off+=chunk){
     size_t n=std::min(chunk,take-off),at=start+off;
     if(at>UINT32_MAX)break;
     if(rd(process.value,uint32_t(at),data.data(),n)){
      for(size_t j=0;j+4<=n;j+=4){
       uint32_t value=0;std::memcpy(&value,data.data()+j,4);
       if(value==anchor)refs.push_back(uint32_t(at+j));
      }
     }
    }
    scanned+=take;
   }
   addr=end;
  }
  std::cout<<"Candidate direct references="<<refs.size()<<" scanned_bytes="<<scanned<<"\n";
 }
 if(refs.empty()){std::cerr<<"No candidate reference found\n";return 5;}
 std::ofstream samples("aoe2hd_building_reference_stages_v3.csv");
 std::ofstream windows("aoe2hd_building_reference_windows_v3.csv");
 if(!samples||!windows)return 5;
 samples<<"stage,selected_address,selected_id,reference_address,reference_value,reference_readable,points_to_A,anchor_alive\n";
 windows<<"stage,reference_address,relative_offset,absolute_address,value,readable\n";
 std::ofstream diag("aoe2hd_building_reference_diagnostics_v3.csv");
 if(!diag)return 5;
 diag<<"stage,attempt,selected_pointer,selected_id,anchor_id,owner_pointer,world_pointer,reason\n";
 const char* phases[]={"A","B","C","D_optional","A_again"};
 const char* instructions[]={"select Centro Urbano 1 (A)","select Quartel (B)",
  "select an Aldeao (C)","select Centro Urbano 2 (D)","reselect Centro Urbano 1 (A)"};
 unsigned completed=0;
 for(size_t phaseIdx=0;phaseIdx<5;++phaseIdx){
  const char* phase=phases[phaseIdx];
  bool captured=false;
  while(!captured){
   std::cout<<"STAGE "<<phase<<": "<<instructions[phaseIdx]
            <<". ENTER capture; r retry; q stop with CSVs.\n";
   if(!std::getline(std::cin,line))return 4;
   if(line=="q"||line=="Q"){
    std::cout<<"Stopped by operator, completed="<<completed<<" phases; partial CSVs saved.\n";
    return 0;
   }
   if(line=="r"||line=="R")continue;
   uint32_t current=0,currentId=0,actual=0,ownerNow=0,worldNow=0;
   bool selectionOk=selected(current,currentId);
   bool anchorOk=w(process.value,anchor,8,actual)&&actual==identity;
   bool ownerOk=w(process.value,base,0x006F2C58,ownerNow)&&ownerNow==owner;
   bool worldOk=ownerOk&&w(process.value,owner,0xc8,worldNow)&&worldNow==world;
   std::string reason="ok";
   if(!ownerOk)reason="owner_changed_or_unreadable";
   else if(!worldOk)reason="world_changed_or_unreadable";
   else if(!anchorOk)reason="anchor_identity_changed_or_unreadable";
   else if(!selectionOk)reason="selection_unavailable";
   diag<<phase<<','<<1<<','<<current<<','<<currentId<<','<<actual<<','
       <<ownerNow<<','<<worldNow<<','<<reason<<"\n";
   diag.flush();
   if(reason!="ok"){
    std::cerr<<"Capture rejected: "<<reason<<"\n";
    if(!ownerOk||!worldOk||!anchorOk){
     std::cerr<<"Context or anchor no longer stable; stopping safely.\n";
     return 4;
    }
    std::cerr<<"Select a single object and press ENTER to retry; q to stop.\n";
    continue;
   }
   if((phaseIdx==0||phaseIdx==4)&&current!=anchor){
    std::cerr<<"Expected original Centro Urbano A; retry this stage.\n";continue;
   }
   if(phaseIdx>=1&&phaseIdx<=3&&current==anchor){
    std::cerr<<"Expected an object distinct from A; retry.\n";continue;
   }
   for(uint32_t ref:refs){
    uint32_t val=0;bool ok=rd(process.value,ref,&val,4);
    samples<<phase<<','<<current<<','<<currentId<<','<<ref<<','<<(ok?val:0)<<','
           <<ok<<','<<(ok&&val==anchor)<<",1\n";
    for(int offset=-64;offset<=64;offset+=4){
     uint32_t at=0,v=0;
     bool safe=offset<0?ref>=uint32_t(-offset):ref<=UINT32_MAX-uint32_t(offset);
     if(safe)at=uint32_t(int64_t(ref)+offset);
     bool readOk=safe&&rd(process.value,at,&v,4);
     windows<<phase<<','<<ref<<','<<offset<<','<<at<<','<<(readOk?v:0)<<','<<readOk<<"\n";
    }
   }
   samples.flush();windows.flush();
   ++completed;captured=true;
   std::cout<<" captured "<<phase<<" selected=0x"<<std::hex<<current<<std::dec
            <<" refs="<<refs.size()<<"\n";
  }
 }
 std::cout<<"BuildingObjectDiscovery v3 completed phases=5 refs="<<refs.size()<<"\n"
          <<"CSV: aoe2hd_building_reference_stages_v3.csv\n"
          <<"CSV: aoe2hd_building_reference_windows_v3.csv\n"\n          <<"CSV: aoe2hd_building_reference_diagnostics_v3.csv\n"
          <<"These observations do not independently establish ownership or collection semantics.\n";
 return 0;
}

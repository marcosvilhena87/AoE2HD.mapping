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
 uint32_t id=pid(),base=id?module(id):0;
 if(!base){std::cerr<<"AoK HD.exe unavailable\n";return 2;}
 Handle process(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,id));
 if(!process.value)return 2;
 uint32_t owner=0,world=0,vt=0,index=0,begin=0,end=0,local=0,object=0,other=0,identity=0;
 if(!w(process.value,base,0x006F2C58,owner)||!owner||!w(process.value,owner,0xc8,world)||
    !w(process.value,world,0,vt)||vt!=base+0x005D329C||
    !w(process.value,world,0x174,index)||!w(process.value,world,0x184,begin)||
    !w(process.value,world,0x188,end)||end<begin||(end-begin)%8||index==0||
    index>=(end-begin)/8||!w(process.value,begin,index*8,local)||
    !w(process.value,local,0x15c,object)||!w(process.value,local,0x160,other)||
    !object||object!=other||!w(process.value,object,8,identity)){
  std::cerr<<"Select one known production building first\n";return 3;
 }
 std::cout<<"Selected building id="<<identity<<" address=0x"<<std::hex<<object<<std::dec<<"\n";
 std::cout<<"Confirm production building (y/n): ";std::string line;
 if(!std::getline(std::cin,line)||(line!="y"&&line!="Y"))return 3;
 // Bounded committed-region scan for direct references to the known object.
 // 64 MiB total byte budget, 4 MiB maximum per region, no writes.
 constexpr size_t budget=64u*1024u*1024u;
 constexpr size_t chunk=64u*1024u;
 std::vector<aoe2hd::ReferenceHitV1> hits;
 size_t scanned=0,regions=0;uintptr_t addr=0x10000;
 while(addr<0x7fff0000&&scanned<budget){
  MEMORY_BASIC_INFORMATION m{};
  if(!VirtualQueryEx(process.value,reinterpret_cast<LPCVOID>(addr),&m,sizeof(m)))break;
  uintptr_t start=uintptr_t(m.BaseAddress),finish=start+m.RegionSize;
  if(finish<=addr)break;
  if(readable(m)&&start<0x80000000){
   size_t cap=std::min<size_t>(size_t(finish-start),4u*1024u*1024u);
   size_t take=std::min(cap,budget-scanned);
   ++regions;
   std::vector<uint8_t> data(chunk);
   for(size_t off=0;off<take;off+=chunk){
    size_t n=std::min(chunk,take-off);
    uintptr_t at=start+off;
    if(at>UINT32_MAX)break;
    if(rd(process.value,uint32_t(at),data.data(),n)){
     auto batch=aoe2hd::FindDirectReferencesV1(uint32_t(at),data.data(),n,object);
     hits.insert(hits.end(),batch.begin(),batch.end());
    }
   }
   scanned+=take;
  }
  addr=finish;
 }
 std::cout<<"Found direct references="<<hits.size()<<" scanned_bytes="<<scanned
          <<" regions="<<regions<<"\n";
 std::cout<<"Select another object (or deselect building) WITHOUT destroying building; press ENTER.\n";
 if(!std::getline(std::cin,line))return 4;
 uint32_t current=0,second=0,objectId=0,ownerNow=0,worldNow=0;
 if(!w(process.value,local,0x15c,current)||!w(process.value,local,0x160,second)||
    !w(process.value,object,8,objectId)||objectId!=identity||
    !w(process.value,base,0x006F2C58,ownerNow)||ownerNow!=owner||
    !w(process.value,owner,0xc8,worldNow)||worldNow!=world){
  std::cerr<<"Game state changed, aborting\n";return 4;
 }
 std::ofstream csv("aoe2hd_building_reference_persistence_v1.csv");
 if(!csv)return 5;
 csv<<"reference_address,reference_value_after,still_points_to_building,within_local_player,within_world\n";
 size_t persistent=0;
 for(const auto& hit:hits){
  uint32_t value=0;bool ok=rd(process.value,hit.address,&value,4);
  bool stable=ok&&aoe2hd::PersistentReferenceV1(hit.target,value,object);
  persistent+=stable;
  csv<<hit.address<<','<<(ok?value:0)<<','<<stable<<','
     <<(hit.address>=local&&uint64_t(hit.address)<uint64_t(local)+0x800)<<','
     <<(hit.address>=world&&uint64_t(hit.address)<uint64_t(world)+0x800)<<"\n";
 }
 auto queue=aoe2hd::ReadProductionQueue(object,
  [&](uint32_t p,uint32_t off,uint32_t& out){return w(process.value,p,off,out);},
  [&](uint32_t p,void* buffer,size_t n){return rd(process.value,p,buffer,n);});
 std::cout<<"BuildingReferenceDiscovery v1 selection_now=0x"<<std::hex<<current<<std::dec
          <<" persistent_refs="<<persistent<<" queue_read_valid="<<queue.valid()
          <<" groups="<<queue.groupCount<<" units="<<queue.unitCount<<"\n"
          <<"CSV: aoe2hd_building_reference_persistence_v1.csv\n"
          <<"Known address was anchored by selection; autonomous discovery NOT yet demonstrated.\n";
 return 0;
}

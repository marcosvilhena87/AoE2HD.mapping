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
 struct Anchor{uint32_t ptr=0,id=0;};
 auto selected=[&](Anchor& a){
  uint32_t x=0,y=0;
  if(!w(process.value,local,0x15c,x)||!w(process.value,local,0x160,y)||!x||x!=y||!w(process.value,x,8,a.id))return false;
  a.ptr=x;return true;
 };
 Anchor A,B;
 std::string line;
 std::cout<<"Select first production building (A), press ENTER.\n";
 if(!std::getline(std::cin,line)||!selected(A))return 4;
 std::cout<<"A: id="<<A.id<<" ptr=0x"<<std::hex<<A.ptr<<std::dec<<" confirm (y/n): ";
 if(!std::getline(std::cin,line)||(line!="y"&&line!="Y"))return 4;
 std::cout<<"Select second DISTINCT production building (B), press ENTER.\n";
 if(!std::getline(std::cin,line)||!selected(B)||B.ptr==A.ptr||B.id==A.id)return 4;
 std::cout<<"B: id="<<B.id<<" ptr=0x"<<std::hex<<B.ptr<<std::dec<<" confirm (y/n): ";
 if(!std::getline(std::cin,line)||(line!="y"&&line!="Y"))return 4;
 constexpr size_t budget=96u*1024u*1024u,perRegion=4u*1024u*1024u,chunk=64u*1024u;
 std::vector<aoe2hd::DualBuildingHitV2> hits;
 size_t scanned=0;uintptr_t addr=0x10000;
 std::vector<uint8_t> buffer(chunk);
 while(addr<0x7fff0000&&scanned<budget){
  MEMORY_BASIC_INFORMATION m{};
  if(!VirtualQueryEx(process.value,reinterpret_cast<LPCVOID>(addr),&m,sizeof(m)))break;
  uintptr_t start=uintptr_t(m.BaseAddress),finish=start+m.RegionSize;
  if(finish<=addr)break;
  if(readable(m)&&start<0x80000000){
   size_t take=std::min({size_t(finish-start),perRegion,budget-scanned});
   for(size_t off=0;off<take;off+=chunk){
    size_t n=std::min(chunk,take-off);
    uintptr_t p=start+off;
    if(p>UINT32_MAX)break;
    if(rd(process.value,uint32_t(p),buffer.data(),n)){
     auto batch=aoe2hd::ScanDualBuildingHitsV2(uint32_t(p),buffer.data(),n,A.ptr,B.ptr);
     hits.insert(hits.end(),batch.begin(),batch.end());
    }
   }
   scanned+=take;
  }
  addr=finish;
 }
 std::cout<<"Scanned="<<scanned<<" direct_hits="<<hits.size()
          <<" both_anchors_present="<<aoe2hd::HasBothAnchorsV2(hits,A.ptr,B.ptr)<<"\n";
 std::cout<<"Now select a THIRD, unrelated object (neither A nor B); press ENTER.\n";
 if(!std::getline(std::cin,line))return 4;
 Anchor C;
 if(!selected(C)||C.ptr==A.ptr||C.ptr==B.ptr){std::cerr<<"Third selection not distinct\n";return 4;}
 uint32_t aid=0,bid=0,ownerNow=0,worldNow=0;
 if(!w(process.value,A.ptr,8,aid)||!w(process.value,B.ptr,8,bid)||
    aid!=A.id||bid!=B.id||!w(process.value,base,0x006F2C58,ownerNow)||
    ownerNow!=owner||!w(process.value,owner,0xc8,worldNow)||worldNow!=world)return 4;
 std::ofstream csv("aoe2hd_building_object_candidates_v2.csv");
 std::ofstream summary("aoe2hd_building_object_summary_v2.csv");
 if(!csv||!summary)return 5;
 csv<<"reference_address,initial_target,after_target,readable,persistent,target_anchor,neighbor_a,neighbor_b,neighbor_both\n";
 size_t keptA=0,keptB=0,neighborBoth=0;
 for(const auto& hit:hits){
  uint32_t after=0;bool ok=rd(process.value,hit.address,&after,4);
  bool keep=ok&&after==hit.target;
  if(keep){keptA+=hit.target==A.ptr;keptB+=hit.target==B.ptr;}
  // Local +/- 128 byte co-occurrence can be incidental; it is only candidate evidence.
  bool nearA=false,nearB=false;
  if(keep){
   uint32_t lo=hit.address>=128?hit.address-128:0x10000;
   for(uint32_t at=lo;at<=hit.address+128&&at>=lo;at+=4){
    uint32_t value=0;
    if(rd(process.value,at,&value,4)){nearA|=value==A.ptr;nearB|=value==B.ptr;}
    if(at>UINT32_MAX-8)break;
   }
  }
  neighborBoth+=keep&&nearA&&nearB;
  csv<<hit.address<<','<<hit.target<<','<<(ok?after:0)<<','<<ok<<','<<keep<<','
     <<(hit.target==A.ptr?"A":"B")<<','<<nearA<<','<<nearB<<','<<(nearA&&nearB)<<"\n";
 }
 auto readQueue=[&](uint32_t building){return aoe2hd::ReadProductionQueue(building,
  [&](uint32_t p,uint32_t off,uint32_t& value){return w(process.value,p,off,value);},
  [&](uint32_t p,void* dst,size_t n){return rd(process.value,p,dst,n);});};
 auto qa=readQueue(A.ptr),qb=readQueue(B.ptr);
 summary<<"anchor,address,id,persistent_refs,queue_valid,groups,units\n";
 summary<<"A,"<<A.ptr<<','<<A.id<<','<<keptA<<','<<qa.valid()<<','<<qa.groupCount<<','<<qa.unitCount<<"\n";
 summary<<"B,"<<B.ptr<<','<<B.id<<','<<keptB<<','<<qb.valid()<<','<<qb.groupCount<<','<<qb.unitCount<<"\n";
 std::cout<<"BuildingObjectDiscovery v2 persistent_A="<<keptA
          <<" persistent_B="<<keptB<<" near_both="<<neighborBoth
          <<" queue_A_valid="<<qa.valid()<<" queue_B_valid="<<qb.valid()<<"\n"
          <<"CSV: aoe2hd_building_object_candidates_v2.csv\n"
          <<"CSV: aoe2hd_building_object_summary_v2.csv\n"
          <<"References or proximity alone DO NOT establish a persistent object collection.\n";
 return 0;
}

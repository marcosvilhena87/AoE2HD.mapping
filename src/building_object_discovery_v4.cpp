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
  uint32_t x=0,y=0;return w(process.value,local,0x15c,x)&&w(process.value,local,0x160,y)&&
   x&&x==y&&w(process.value,x,8,id)?(ptr=x,true):false;
 };
 uint32_t anchor=0,anchorId=0;std::string line;
 std::cout<<"Select Centro Urbano A, press ENTER.\n";
 if(!std::getline(std::cin,line)||!selected(anchor,anchorId))return 4;
 std::cout<<"A id="<<anchorId<<" ptr=0x"<<std::hex<<anchor<<std::dec<<" confirm y/n: ";
 if(!std::getline(std::cin,line)||(line!="y"&&line!="Y"))return 4;
 std::cout<<"Reference address in hex (0 = scan for current A): ";
 if(!std::getline(std::cin,line))return 4;
 uint32_t ref=0;
 try{unsigned long value=std::stoul(line.empty()?"0":line,nullptr,16);if(value>UINT32_MAX)return 4;ref=uint32_t(value);}
 catch(...){return 4;}
 if(!ref){
  constexpr size_t budget=96u*1024u*1024u,chunk=64u*1024u,regionCap=4u*1024u*1024u;
  size_t scanned=0;uintptr_t address=0x10000;std::vector<uint8_t> data(chunk);
  std::vector<uint32_t> refs;
  while(address<0x7fff0000&&scanned<budget){
   MEMORY_BASIC_INFORMATION m{};
   if(!VirtualQueryEx(process.value,reinterpret_cast<LPCVOID>(address),&m,sizeof(m)))break;
   uintptr_t start=uintptr_t(m.BaseAddress),end=start+m.RegionSize;
   if(end<=address)break;
   if(readable(m)&&start<0x80000000){
    size_t nbytes=std::min({size_t(end-start),regionCap,budget-scanned});
    for(size_t off=0;off<nbytes;off+=chunk){
     size_t n=std::min(chunk,nbytes-off),at=start+off;if(at>UINT32_MAX)break;
     if(rd(process.value,uint32_t(at),data.data(),n))
      for(size_t j=0;j+4<=n;j+=4){
       uint32_t v=0;std::memcpy(&v,data.data()+j,4);
       if(v==anchor)refs.push_back(uint32_t(at+j));
      }
    }scanned+=nbytes;
   }address=end;
  }
  std::cout<<"Candidates to A="<<refs.size()<<" scanned="<<scanned<<"\n";
  for(uint32_t candidate:refs)std::cout<<"  0x"<<std::hex<<candidate<<std::dec<<"\n";
  std::cout<<"Choose reference hex from list (not selection reference): ";
  if(!std::getline(std::cin,line))return 4;
  try{unsigned long v=std::stoul(line,nullptr,16);if(v>UINT32_MAX)return 4;ref=uint32_t(v);}
  catch(...){return 4;}
  if(std::find(refs.begin(),refs.end(),ref)==refs.end()){std::cerr<<"Not a scanned candidate\n";return 4;}
 }
 if(ref<0x10800||ref>UINT32_MAX-2048||(ref&3u)){std::cerr<<"Invalid reference address\n";return 4;}
 uint32_t windowStart=ref-2048;
 std::ofstream stages("aoe2hd_building_reference_stages_v4.csv");
 std::ofstream words("aoe2hd_building_reference_window_words_v4.csv");
 if(!stages||!words)return 5;
 stages<<"stage,selected_ptr,selected_id,anchor_id,ref_value,ref_points_to_A,window_readable,anchor_alive\n";
 words<<"stage,relative_offset,address,value,points_to_A,points_to_selection,looks_like_object,object_id\n";
 const char* phases[]={"A","B","C","D","A_again"};
 const char* instructions[]={"Centro Urbano A","Quartel B","Aldeao C","Centro Urbano D","Centro Urbano A"};
 uint32_t captured[5]={};size_t completed=0;
 for(size_t i=0;i<5;++i){
  std::cout<<"STAGE "<<phases[i]<<" select "<<instructions[i]<<" then ENTER; q quit.\n";
  if(!std::getline(std::cin,line))return 4;
  if(line=="q"||line=="Q")break;
  uint32_t current=0,currentId=0,aId=0,o=0,wo=0;
  bool ok=selected(current,currentId)&&w(process.value,anchor,8,aId)&&aId==anchorId&&
   w(process.value,base,0x006F2C58,o)&&o==owner&&w(process.value,owner,0xc8,wo)&&wo==world;
  if(!ok){std::cerr<<"Invalid selection/anchor/world; partial CSV retained\n";return 4;}
  if((i==0||i==4)&&current!=anchor){std::cerr<<"Expected Centro Urbano A\n";return 4;}
  uint32_t referenceValue=0;bool refOk=rd(process.value,ref,&referenceValue,4);
  std::vector<uint8_t> data(4096);
  bool windowOk=rd(process.value,windowStart,data.data(),data.size());
  stages<<phases[i]<<','<<current<<','<<currentId<<','<<aId<<','<<(refOk?referenceValue:0)<<','
        <<(refOk&&referenceValue==anchor)<<','<<windowOk<<",1\n";
  if(windowOk){
   auto decoded=aoe2hd::DecodeWindowV4(data);
   for(const auto& entry:decoded){
    uint32_t objectId=0;
    bool plaus=entry.value>=0x10000&&!(entry.value&3u)&&
       w(process.value,entry.value,8,objectId)&&objectId>0&&objectId<10000000;
    words<<phases[i]<<','<<(int64_t(entry.offset)-2048)<<','
         <<(uint64_t(windowStart)+entry.offset)<<','<<entry.value<<','
         <<(entry.value==anchor)<<','<<(entry.value==current)<<','<<plaus<<','
         <<(plaus?objectId:0)<<"\n";
   }
  }
  stages.flush();words.flush();captured[i]=current;++completed;
  std::cout<<" captured "<<phases[i]<<" ref=0x"<<std::hex<<referenceValue<<std::dec
           <<" window_readable="<<windowOk<<"\n";
 }
 std::cout<<"BuildingObjectDiscovery v4 completed="<<completed
          <<" reference=0x"<<std::hex<<ref<<std::dec<<"\n"
          <<"CSV: aoe2hd_building_reference_stages_v4.csv\n"
          <<"CSV: aoe2hd_building_reference_window_words_v4.csv\n"
          <<"Pointer-like values and plausible IDs are hypotheses, not verified object collection entries.\n";
 return 0;
}

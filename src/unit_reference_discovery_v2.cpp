#include "UnitReferenceDiscovery.h"
#include "ProcessRegionScanner.h"
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
namespace {
struct Handle{HANDLE h;explicit Handle(HANDLE x):h(x){}~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}};
bool rd(HANDLE h,uint32_t addr,void* dest,size_t n){SIZE_T got=0;return ReadProcessMemory(h,reinterpret_cast<LPCVOID>(uintptr_t(addr)),dest,n,&got)&&got==n;}
bool u32(HANDLE h,uint32_t p,uint32_t off,uint32_t& out){return p<=UINT32_MAX-off&&rd(h,p+off,&out,4);}
uint32_t pid(){
 Handle h(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));
 if(h.h==INVALID_HANDLE_VALUE)return 0;
 PROCESSENTRY32W e{};e.dwSize=sizeof(e);
 if(!Process32FirstW(h.h,&e))return 0;
 do{if(_wcsicmp(e.szExeFile,L"AoK HD.exe")==0)return e.th32ProcessID;}while(Process32NextW(h.h,&e));
 return 0;
}
uint32_t module(uint32_t id){
 Handle h(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,id));
 if(h.h==INVALID_HANDLE_VALUE)return 0;
 MODULEENTRY32W e{};e.dwSize=sizeof(e);
 if(!Module32FirstW(h.h,&e))return 0;
 do{if(_wcsicmp(e.szModule,L"AoK HD.exe")==0){auto x=uintptr_t(e.modBaseAddr);return x<=UINT32_MAX?uint32_t(x):0;}}
 while(Module32NextW(h.h,&e));
 return 0;
}
struct Anchor{uint32_t owner=0,world=0,local=0,object=0,id=0,ownerCandidate=0,table=0;float hp=0;};
bool acquire(HANDLE h,uint32_t base,Anchor& a,std::string& reason){
 uint32_t owner=0,world=0,vt=0,idx=0,begin=0,end=0,cap=0;
 if(base>UINT32_MAX-0x006F2C58||!u32(h,base,0x006F2C58,owner)||!owner||
 !u32(h,owner,0xC8,world)||!world||!u32(h,world,0,vt)||
 vt!=base+0x005D329C||!u32(h,world,0x174,idx)||
 !u32(h,world,0x184,begin)||!u32(h,world,0x188,end)||
 !u32(h,world,0x18C,cap)||!begin||end<begin||cap<end||
 (end-begin)%8||(cap-begin)%8||(end-begin)/8<2||
 (cap-begin)/8>9||idx==0||idx>=(end-begin)/8){
 reason="world_not_ready";return false;
 }
 uint32_t local=0,control=0,pvt=0,back=0,slotA=0,slotB=0;
 if(!u32(h,begin,idx*8,local)||!u32(h,begin,idx*8+4,control)||
 !local||control>UINT32_MAX-12||local!=control+12||
 !u32(h,local,0,pvt)||pvt!=base+0x005DCAE4||
 !u32(h,local,4,back)||back!=world){
 reason="local_player_invalid";return false;
 }
 if(!u32(h,local,0x15c,slotA)||!u32(h,local,0x160,slotB)){
 reason="selection_unreadable";return false;
 }
 if(!slotA||slotA!=slotB){reason="select_exactly_one_unit";return false;}
 if(slotA<0x10000||(slotA&3u)||slotA>UINT32_MAX-0x38){
 reason="selection_invalid";return false;
 }
 uint32_t words[5]{},hpBits=0;
 if(!rd(h,slotA,words,sizeof(words))||!u32(h,slotA,0x34,hpBits)){
 reason="selected_object_unreadable";return false;
 }
 float hp;std::memcpy(&hp,&hpBits,4);
 if(!std::isfinite(hp)||hp<0||hp>100000){
 reason="selected_hp_invalid";return false;
 }
 uint32_t againA=0,againB=0,worldAgain=0,ownerAgain=0;
 if(!u32(h,local,0x15c,againA)||!u32(h,local,0x160,againB)||
 !u32(h,owner,0xC8,worldAgain)||!u32(h,base,0x006F2C58,ownerAgain)||
 againA!=slotA||againB!=slotB||worldAgain!=world||ownerAgain!=owner){
 reason="selection_changed";return false;
 }
 a={owner,world,local,slotA,words[2],words[4],words[3],hp};
 return true;
}
struct Region{std::string name;uint32_t offset=0,base=0,bytes=0,stride=0;};
std::vector<Region> regions(HANDLE h,const Anchor& a){
 std::vector<Region> result={{"World",0,a.world,0x800,4},{"LocalPlayer",0,a.local,0x800,4}};
 std::set<std::pair<uint32_t,uint32_t>> buffers;
 for(const auto& root:std::vector<std::pair<const char*,uint32_t>>{{"World",a.world},{"LocalPlayer",a.local}}){
  for(uint32_t off=0x10;off+12<=0x800;off+=4){
   uint32_t w[3]{};
   if(root.second>UINT32_MAX-off-12||!rd(h,root.second+off,w,12))continue;
   auto b=w[0],e=w[1],c=w[2];
   if(b<0x10000||b%4||e<=b||c<e||c-b>8192||e-b>2048)continue;
   if(!buffers.insert({b,e-b}).second)continue;
   result.push_back({std::string(root.first)+"Vector",off,b,e-b,4});
  }
 }
 return result;
}
}
int main(int argc,char** argv){
 uint64_t maxBytes=64ull*1024*1024;
 uint32_t maxSeconds=15,maxHits=1024;
 for(int i=1;i<argc;i++){
  std::string a=argv[i];
  if(i+1>=argc){std::cerr<<"Usage: aoe2hd_unit_reference_discovery_v2 [--max-mib 1..256] [--max-seconds 1..60] [--max-hits 1..4096]\n";return 2;}
  char* end=nullptr; unsigned long n=std::strtoul(argv[++i],&end,10);
  if(!end||*end||n==0){std::cerr<<"Invalid option value\n";return 2;}
  if(a=="--max-mib"&&n<=256)maxBytes=uint64_t(n)*1024*1024;
  else if(a=="--max-seconds"&&n<=60)maxSeconds=static_cast<uint32_t>(n);
  else if(a=="--max-hits"&&n<=4096)maxHits=static_cast<uint32_t>(n);
  else {std::cerr<<"Unknown or out-of-range option\n";return 2;}
 }

 uint32_t id=pid(),base=id?module(id):0;
 if(!base){std::cerr<<"AoK HD.exe not found.\n";return 1;}
 Handle h(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,id));
 if(!h.h){std::cerr<<"Cannot open game for read-only memory access.\n";return 1;}
 Anchor a;std::string reason;
 if(!acquire(h.h,base,a,reason)){std::cerr<<"Anchor unavailable: "<<reason<<"\n";return 2;}
 auto scan=regions(h.h,a);
 // Heap-wide bounded search uses VirtualQueryEx; no remote writes.
 // Selected anchor remains constant until final validation.
 std::ofstream wide("aoe2hd_unit_references_v2.csv",std::ios::trunc);
 if(!wide){std::cerr<<"Failed to create v2 CSV\n";return 3;}
 wide<<"pid,world,local,anchor,anchor_id,region_base,region_size,protection,address,before,after,selection_slot,status\n";
 uint64_t consumed=0;uint32_t wideHits=0,regionsSeen=0;
 auto started=std::chrono::steady_clock::now();
 uintptr_t address=0x10000;
 std::vector<uint8_t> chunk(64*1024);
 while(address<0x7FFF0000u&&consumed<maxBytes&&wideHits<maxHits){
  if(std::chrono::steady_clock::now()-started>=std::chrono::seconds(maxSeconds))break;
  MEMORY_BASIC_INFORMATION mbi{};
  if(!VirtualQueryEx(h.h,reinterpret_cast<LPCVOID>(address),&mbi,sizeof(mbi)))break;
  uintptr_t regionBase=reinterpret_cast<uintptr_t>(mbi.BaseAddress);
  uintptr_t regionEnd=regionBase+mbi.RegionSize;
  if(regionEnd<=address)break;
  uintptr_t next=regionEnd;
  const DWORD protection=mbi.Protect&0xFF;
  bool readable=(mbi.State==MEM_COMMIT)&&!(mbi.Protect&PAGE_GUARD)&&
     !(mbi.Protect&PAGE_NOACCESS) &&
     (protection==PAGE_READONLY||protection==PAGE_READWRITE||
      protection==PAGE_WRITECOPY||protection==PAGE_EXECUTE_READ||
      protection==PAGE_EXECUTE_READWRITE||protection==PAGE_EXECUTE_WRITECOPY);
  if(readable){
   ++regionsSeen;
   uintptr_t current=std::max(address,regionBase);
   while(current<regionEnd&&consumed<maxBytes&&wideHits<maxHits){
    if(std::chrono::steady_clock::now()-started>=std::chrono::seconds(maxSeconds))break;
    const size_t n=static_cast<size_t>(std::min<uint64_t>(
      {static_cast<uint64_t>(chunk.size()),static_cast<uint64_t>(regionEnd-current),
       maxBytes-consumed}));
    if(n<4)break;
    SIZE_T got=0;
    BOOL ok=ReadProcessMemory(h.h,reinterpret_cast<LPCVOID>(current),chunk.data(),n,&got);
    // Account for attempted reads even when a page races with the scan.
    consumed+=n;
    if(ok&&got>=4){
     auto hits=aoe2hd::SearchPointerChunk(static_cast<uint32_t>(current),
       chunk.data(),got,a.object,maxHits-wideHits);
     for(const auto& hit:hits){
      const bool slot=hit.address==a.local+0x15c||hit.address==a.local+0x160;
      wide<<id<<','<<a.world<<','<<a.local<<','<<a.object<<','<<a.id<<','
          <<regionBase<<','<<mbi.RegionSize<<','<<mbi.Protect<<','
          <<hit.address<<','<<hit.before<<','<<hit.after<<','<<slot<<",hypothesis\n";
      ++wideHits;
     }
    }
    current+=n;
   }
  }
  address=next;
 }
 wide.flush();

 auto reader=[&](uint32_t addr,void* dest,size_t bytes){return rd(h.h,addr,dest,bytes);};
 std::vector<aoe2hd::ReferenceHit> hits;
 std::ofstream csv("aoe2hd_unit_references.csv",std::ios::trunc);
 if(!csv){std::cerr<<"CSV creation failed.\n";return 3;}
 csv<<"pid,world,local,anchor,anchor_id,anchor_owner_candidate,anchor_hp,source,source_offset,region_base,region_bytes,address,entry_index,word,path,value,target,status\n";
 for(const auto& r:scan){
  auto found=aoe2hd::FindReferences(reader,r.base,r.bytes,a.object,r.name.c_str());
  for(auto& x:found){
   uint32_t target=x.path==std::string("direct")?x.value:a.object;
   csv<<id<<','<<a.world<<','<<a.local<<','<<a.object<<','<<a.id<<','
    <<a.ownerCandidate<<','<<a.hp<<','<<r.name<<','<<r.offset<<','
    <<r.base<<','<<r.bytes<<','<<x.address<<','<<x.index<<','<<x.word<<','
    <<x.path<<','<<x.value<<','<<target<<",hypothesis\n";
  }
  hits.insert(hits.end(),found.begin(),found.end());
 }
 Anchor after;std::string reasonAfter;
 if(!acquire(h.h,base,after,reasonAfter)||after.world!=a.world||
 after.local!=a.local||after.object!=a.object||after.id!=a.id){
  csv.close();wide.close();
  std::remove("aoe2hd_unit_references_v2.csv");
  std::remove("aoe2hd_unit_references.csv");
  std::cerr<<"Selection/world changed during scan; incomplete CSVs removed.\n";
  return 4;
 }
 std::cout<<"V2 regions="<<regionsSeen<<" bytes_scanned="<<consumed<<" wide_hits="<<wideHits<<"\nCSV: aoe2hd_unit_references_v2.csv\n";
 std::cout<<"Selected anchor: 0x"<<std::hex<<a.object<<std::dec
  <<" candidate_id="<<a.id<<" hp="<<a.hp<<"\n"
  <<"Regions scanned="<<scan.size()<<" hits="<<hits.size()<<"\n"
  <<"CSV: aoe2hd_unit_references.csv\n"
  <<"References are hypotheses, not confirmed unit collections.\n";
 return 0;
}

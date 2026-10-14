#include "UnitReferenceDiscovery.h"
#include "ReferencePersistence.h"
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

struct ScanRegion {uint32_t base=0,size=0,protect=0;int priority=0;};
bool readable(const MEMORY_BASIC_INFORMATION& m){
 DWORD p=m.Protect&0xff;
 return m.State==MEM_COMMIT && !(m.Protect&PAGE_GUARD) &&
 (p==PAGE_READONLY||p==PAGE_READWRITE||p==PAGE_WRITECOPY||
 p==PAGE_EXECUTE_READ||p==PAGE_EXECUTE_READWRITE||p==PAGE_EXECUTE_WRITECOPY);
}
std::vector<ScanRegion> enumerate(HANDLE h,const Anchor& a){
 std::vector<ScanRegion> out;
 uintptr_t addr=0x10000;
 while(addr<0x7fff0000){
  MEMORY_BASIC_INFORMATION m{};
  if(!VirtualQueryEx(h,reinterpret_cast<LPCVOID>(addr),&m,sizeof(m)))break;
  uintptr_t base=reinterpret_cast<uintptr_t>(m.BaseAddress),end=base+m.RegionSize;
  if(end<=addr)break;
  if(readable(m)&&base<=UINT32_MAX&&m.RegionSize<=UINT32_MAX){
   bool key=(a.world>=base&&a.world<end)||(a.local>=base&&a.local<end)||
            (a.object>=base&&a.object<end);
   out.push_back({static_cast<uint32_t>(base),static_cast<uint32_t>(m.RegionSize),
                  m.Protect,key?0:1});
  }
  addr=end;
 }
 std::stable_sort(out.begin(),out.end(),[](const ScanRegion& x,const ScanRegion& y){
  if(x.priority!=y.priority)return x.priority<y.priority;
  return x.base<y.base;
 });
 return out;
}
struct Hit {uint32_t address=0,region=0,regionSize=0,protect=0,before=0,after=0;};
int main(int argc,char** argv){
 uint64_t maxBytes=64ull*1024*1024;
 uint32_t maxSeconds=15,maxHits=1024;
 for(int i=1;i<argc;i++){
  const std::string option=argv[i];
  if(i+1>=argc){std::cerr<<"Missing option value\n";return 2;}
  char* end=nullptr;unsigned long v=std::strtoul(argv[++i],&end,10);
  if(!end||*end||v==0){std::cerr<<"Invalid value\n";return 2;}
  if(option=="--max-mib"&&v<=256)maxBytes=uint64_t(v)*1024*1024;
  else if(option=="--max-seconds"&&v<=60)maxSeconds=static_cast<uint32_t>(v);
  else if(option=="--max-hits"&&v<=4096)maxHits=static_cast<uint32_t>(v);
  else{std::cerr<<"Invalid option or range\n";return 2;}
 }
 uint32_t id=pid(),base=id?module(id):0;
 if(!base){std::cerr<<"AoK HD.exe not found\n";return 1;}
 Handle h(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,id));
 if(!h.h){std::cerr<<"OpenProcess failed\n";return 1;}
 Anchor a;std::string reason;
 if(!acquire(h.h,base,a,reason)){std::cerr<<"Anchor: "<<reason<<"\n";return 2;}
 const auto map=enumerate(h.h,a);
 std::vector<Hit> hits;
 std::vector<uint8_t> buf(64*1024);
 auto start=std::chrono::steady_clock::now();
 uint64_t scanned=0;uint32_t regionsUsed=0;
 bool timedOut=false;
 for(const auto& region:map){
  if(scanned>=maxBytes||hits.size()>=maxHits)break;
  ++regionsUsed;
  uint64_t offset=0;
  while(offset<region.size&&scanned<maxBytes&&hits.size()<maxHits){
   if(std::chrono::steady_clock::now()-start>=std::chrono::seconds(maxSeconds)){
    timedOut=true;break;
   }
   size_t n=static_cast<size_t>(std::min<uint64_t>(
     {buf.size(),uint64_t(region.size)-offset,maxBytes-scanned}));
   if(n<4)break;
   uint32_t address=region.base+static_cast<uint32_t>(offset);
   SIZE_T got=0;
   BOOL ok=ReadProcessMemory(h.h,reinterpret_cast<LPCVOID>(uintptr_t(address)),buf.data(),n,&got);
   scanned+=n;
   if(ok&&got>=4){
    auto found=aoe2hd::SearchPointerChunk(address,buf.data(),got,a.object,
                      static_cast<uint32_t>(maxHits-hits.size()));
    for(const auto& hit:found)
     hits.push_back({hit.address,region.base,region.size,region.protect,hit.before,hit.after});
   }
   offset+=n;
  }
  if(timedOut)break;
 }
 Anchor beforeSwitch;std::string r;
 if(!acquire(h.h,base,beforeSwitch,r)||beforeSwitch.object!=a.object||
    beforeSwitch.world!=a.world||beforeSwitch.local!=a.local||
    beforeSwitch.id!=a.id){
  std::cerr<<"Initial selection changed during scan; result discarded\n";return 4;
 }
 std::cout<<"V3 regions="<<regionsUsed<<" bytes_scanned="<<scanned
          <<" hits="<<hits.size()<<" timeout="<<timedOut<<"\n";
 std::cout<<"Anchor 0x"<<std::hex<<a.object<<std::dec<<" candidate_id="<<a.id<<"\n";
 std::cout<<"Now select a DIFFERENT villager, then press ENTER to test persistence.\n";
 std::string line;std::getline(std::cin,line);
 Anchor b;std::string secondReason;
 if(!acquire(h.h,base,b,secondReason)||b.world!=a.world||
    b.local!=a.local||b.object==a.object){
  std::cerr<<"Different selected unit required, same world/local. No CSV written: "
           <<secondReason<<"\n";return 5;
 }
 std::ofstream csv("aoe2hd_unit_references_v3.csv",std::ios::trunc);
 if(!csv){std::cerr<<"CSV open failed\n";return 3;}
 csv<<"pid,world,local,anchor,anchor_id,second_anchor,second_id,region_base,region_size,protection,address,offset_in_region,before,after,selection_slot,observed_value_after,persisted,nearby_minus8,nearby_plus8,status\n";
 uint32_t persistent=0;
 for(const auto& hit:hits){
  uint32_t now=0,minus8=0,plus8=0;
  bool ok=rd(h.h,hit.address,&now,4);
  bool persist=aoe2hd::IsPersistentReference(ok!=FALSE,now,a.object);
  if(persist)++persistent;
  if(hit.address>=8)rd(h.h,hit.address-8,&minus8,4);
  if(hit.address<=UINT32_MAX-8)rd(h.h,hit.address+8,&plus8,4);
  bool selection=aoe2hd::IsSelectionSlot(hit.address,a.local);
  csv<<id<<','<<a.world<<','<<a.local<<','<<a.object<<','<<a.id<<','
     <<b.object<<','<<b.id<<','<<hit.region<<','<<hit.regionSize<<','
     <<hit.protect<<','<<hit.address<<','<<(hit.address-hit.region)<<','
     <<hit.before<<','<<hit.after<<','<<selection<<',';
  if(ok)csv<<now;
  csv<<','<<persist<<','<<minus8<<','<<plus8<<",hypothesis\n";
 }
 std::cout<<"Second anchor 0x"<<std::hex<<b.object<<std::dec
          <<" id="<<b.id<<" persisted="<<persistent<<"/"<<hits.size()<<"\n"
          <<"CSV: aoe2hd_unit_references_v3.csv\n"
          <<"Persistence alone does not establish a unit collection.\n";
 return 0;
}

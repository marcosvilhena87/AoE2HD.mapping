#include "UnitReferenceDiscovery.h"
#include "DualAnchorDiscovery.h"
#include <map>
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
 uint64_t budget=64ull*1024*1024;
 uint32_t seconds=15,maxHits=2048;
 for(int i=1;i<argc;++i){
  std::string opt=argv[i];
  if(i+1>=argc){std::cerr<<"Missing value for "<<opt<<"\n";return 2;}
  char* end=nullptr;unsigned long val=std::strtoul(argv[++i],&end,10);
  if(!end||*end||!val){std::cerr<<"Invalid numeric argument\n";return 2;}
  if(opt=="--max-mib"&&val<=256)budget=uint64_t(val)*1024*1024;
  else if(opt=="--max-seconds"&&val<=60)seconds=uint32_t(val);
  else if(opt=="--max-hits"&&val<=8192)maxHits=uint32_t(val);
  else{std::cerr<<"Invalid option or range\n";return 2;}
 }
 uint32_t id=pid(),base=id?module(id):0;
 if(!base){std::cerr<<"AoK HD.exe not found\n";return 1;}
 Handle process(OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION,FALSE,id));
 if(!process.h){std::cerr<<"OpenProcess failed\n";return 1;}
 Anchor a,b;std::string reason;
 if(!acquire(process.h,base,a,reason)){std::cerr<<"First anchor: "<<reason<<"\n";return 2;}
 std::cout<<"First unit 0x"<<std::hex<<a.object<<std::dec<<" id="<<a.id<<"\n"
          <<"Select a DIFFERENT villager, then press ENTER.\n";
 std::string line;std::getline(std::cin,line);
 if(!acquire(process.h,base,b,reason)||b.world!=a.world||b.local!=a.local||
    b.object==a.object){std::cerr<<"Second anchor invalid or session changed: "<<reason<<"\n";return 3;}
 // The old anchor must remain a readable object with its original identity.
 uint32_t firstId=0;
 if(!u32(process.h,a.object,8,firstId)||firstId!=a.id){
  std::cerr<<"First anchor no longer has stable candidate ID; aborting\n";return 3;
 }
 auto regions=enumerate(process.h,b);
 std::vector<aoe2hd::DualHit> hits;
 std::vector<uint8_t> buffer(64*1024);
 uint64_t scanned=0;uint32_t visited=0;
 auto start=std::chrono::steady_clock::now();
 bool timeout=false;
 for(const auto& region:regions){
  if(scanned>=budget||hits.size()>=maxHits)break;
  ++visited;
  for(uint64_t offset=0;offset<region.size&&scanned<budget&&hits.size()<maxHits;){
   if(std::chrono::steady_clock::now()-start>=std::chrono::seconds(seconds)){
    timeout=true;break;
   }
   const size_t n=static_cast<size_t>(std::min<uint64_t>(
    {buffer.size(),uint64_t(region.size)-offset,budget-scanned}));
   if(n<4)break;
   const uint32_t address=region.base+static_cast<uint32_t>(offset);
   SIZE_T got=0;
   BOOL ok=ReadProcessMemory(process.h,reinterpret_cast<LPCVOID>(uintptr_t(address)),
                             buffer.data(),n,&got);
   scanned+=n;
   if(ok&&got>=4){
    auto found=aoe2hd::SearchTwoAnchors(address,buffer.data(),got,a.object,b.object,
                                        maxHits-hits.size());
    hits.insert(hits.end(),found.begin(),found.end());
   }
   offset+=n;
  }
  if(timeout)break;
 }
 Anchor verify;std::string verifyReason;
 if(!acquire(process.h,base,verify,verifyReason)||verify.object!=b.object||
    verify.id!=b.id||verify.world!=a.world||verify.local!=a.local||
    !u32(process.h,a.object,8,firstId)||firstId!=a.id){
  std::cerr<<"Anchor changed during scan; results discarded\n";return 4;
 }
 // Pairs are structural hypotheses. Ignore direct selection slots.
 std::vector<aoe2hd::DualHit> candidateHits;
 for(const auto& h:hits)
  if(!aoe2hd::IsSelectionSlot(h.address,a.local))candidateHits.push_back(h);
 const auto pairs=aoe2hd::FindNearbyPairs(candidateHits,a.object,b.object,256,4096);
 std::ofstream out("aoe2hd_unit_references_v4.csv");
 if(!out){std::cerr<<"Failed to create CSV\n";return 5;}
 out<<"pid,world,local,first_anchor,first_id,second_anchor,second_id,address,value,anchor_role,selection_slot,previous_word,next_word,status\n";
 for(const auto& h:hits){
  uint32_t prev=0,next=0;
  if(h.address>=4)u32(process.h,h.address-4,0,prev);
  if(h.address<=UINT32_MAX-8)u32(process.h,h.address+4,0,next);
  out<<id<<','<<a.world<<','<<a.local<<','<<a.object<<','<<a.id<<','
     <<b.object<<','<<b.id<<','<<h.address<<','<<h.value<<','
     <<(h.value==a.object?"first":"second")<<','
     <<aoe2hd::IsSelectionSlot(h.address,a.local)<<','<<prev<<','<<next
     <<",hypothesis\n";
 }
 std::ofstream pairCsv("aoe2hd_unit_reference_pairs_v4.csv");
 if(!pairCsv){std::cerr<<"Failed to create pair CSV\n";return 5;}
 pairCsv<<"first_reference,second_reference,distance,first_anchor,second_anchor,status\n";
 for(const auto& p:pairs)
  pairCsv<<p.first<<','<<p.second<<','<<p.distance<<','
         <<a.object<<','<<b.object<<",hypothesis\n";
 std::cout<<"V4 regions="<<visited<<" bytes_scanned="<<scanned
          <<" hits="<<hits.size()<<" pairs="<<pairs.size()
          <<" timeout="<<timeout<<"\n"
          <<"CSV: aoe2hd_unit_references_v4.csv\n"
          <<"CSV: aoe2hd_unit_reference_pairs_v4.csv\n"
          <<"Pairs are hypotheses, not unit collections.\n";
 return 0;
}

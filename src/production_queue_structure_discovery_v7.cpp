#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include "ProductionQueueEnumerationV7.h"
#include "ProductionQueueReader.h"
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
namespace {
struct Handle {HANDLE h;explicit Handle(HANDLE x):h(x){}~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}};
bool read(HANDLE h,uint32_t p,void* out,size_t n){SIZE_T got=0;return ReadProcessMemory(h,reinterpret_cast<LPCVOID>(uintptr_t(p)),out,n,&got)!=0&&got==n;}
bool word(HANDLE h,uint32_t p,uint32_t offset,uint32_t& out){return p<=UINT32_MAX-offset&&read(h,p+offset,&out,4);}
uint32_t processId(){
 Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));
 if(snap.h==INVALID_HANDLE_VALUE)return 0;
 PROCESSENTRY32W p{};p.dwSize=sizeof(p);
 if(!Process32FirstW(snap.h,&p))return 0;
 do{if(_wcsicmp(p.szExeFile,L"AoK HD.exe")==0)return p.th32ProcessID;}while(Process32NextW(snap.h,&p));
 return 0;
}
uint32_t moduleBase(uint32_t pid){
 Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid));
 if(snap.h==INVALID_HANDLE_VALUE)return 0;
 MODULEENTRY32W m{};m.dwSize=sizeof(m);
 if(!Module32FirstW(snap.h,&m))return 0;
 do{if(_wcsicmp(m.szModule,L"AoK HD.exe")==0)return uint32_t(uintptr_t(m.modBaseAddr));}while(Module32NextW(snap.h,&m));
 return 0;
}
bool parseSequence(const std::string& s,std::vector<uint16_t>& out){
 if(s=="empty"||s=="-")return true;
 size_t i=0;
 while(i<s.size()){
  size_t stop=s.find('|',i);
  auto token=s.substr(i,stop==std::string::npos?std::string::npos:stop-i);
  if(token=="M"||token=="Militia")out.push_back(74);
  else if(token=="S"||token=="Spearman")out.push_back(93);
  else if(token=="V"||token=="Villager")out.push_back(83);
  else return false;
  if(stop==std::string::npos)break;
  i=stop+1;
 }
 return !out.empty()&&s.back()!='|';
}
std::string join(const std::vector<uint16_t>& v){
 std::string out;for(uint16_t n:v){if(!out.empty())out+='|';out+=std::to_string(n);}return out;
}
}
int main(){
 uint32_t pid=processId(),base=pid?moduleBase(pid):0;
 if(!base){std::cerr<<"AoK HD.exe not found\n";return 2;}
 Handle h(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,pid));
 if(!h.h)return 2;
 uint32_t owner=0,world=0,vt=0,index=0,begin=0,end=0,local=0,selection=0,selection2=0;
 if(!word(h.h,base,0x006F2C58,owner)||!owner||!word(h.h,owner,0xC8,world)||
    !word(h.h,world,0,vt)||vt!=base+0x005D329C||
    !word(h.h,world,0x174,index)||!word(h.h,world,0x184,begin)||
    !word(h.h,world,0x188,end)||end<begin||!begin||(end-begin)%8||
    index==0||index>=(end-begin)/8||!word(h.h,begin,index*8,local)||
    !local||!word(h.h,local,0x15C,selection)||
    !word(h.h,local,0x160,selection2)||!selection||selection!=selection2){
  std::cerr<<"World/player/selected building unavailable\n";return 3;
 }
 uint32_t selectedId=0;
 if(!word(h.h,selection,8,selectedId))return 3;
 std::string line;
 std::cout<<"Selected building object ID="<<selectedId<<" at 0x"<<std::hex<<selection<<std::dec<<"\n"
          <<"Confirm production BUILDING; do not research technologies (y/n): ";
 if(!std::getline(std::cin,line)||(line!="y"&&line!="Y"))return 3;
 auto stable=[&](){uint32_t x=0,y=0,id=0,w=0,o=0;return
  word(h.h,local,0x15c,x)&&word(h.h,local,0x160,y)&&x==selection&&y==selection&&
  word(h.h,selection,8,id)&&id==selectedId&&
  word(h.h,owner,0xC8,w)&&w==world&&word(h.h,base,0x006F2C58,o)&&o==owner;};
 std::ofstream rows("aoe2hd_production_queue_enumeration_v7.csv");
 std::ofstream groups("aoe2hd_production_queue_groups_v7.csv");
 if(!rows||!groups)return 5;
 rows<<"stage,visual,operator_verified,pointer,readable,group_count,unit_count,structural_valid,reconstructed_ids,visual_match,independent_verified\n";
 groups<<"stage,index,type_id,quantity\n";
 const char* stages[]={"empty","first","second","third","fourth","cancel","finish1","finish2"};
 unsigned valid=0,matched=0;
 for(const char* stage:stages){
  std::cout<<"STAGE "<<stage<<": adjust unit queue, then press ENTER.\n";
  if(!std::getline(std::cin,line)||!stable()){std::cerr<<"Selection/session changed\n";return 4;}
  std::string visual;std::vector<uint16_t> expected;
  std::cout<<"Visual sequence (M|S|S|M, V for Villager, empty): ";
  if(!std::getline(std::cin,visual)||!parseSequence(visual,expected)){std::cerr<<"Bad sequence\n";return 4;}
  std::cout<<"Visually verified (y/n): ";
  if(!std::getline(std::cin,line))return 4;
  bool verified=line=="y"||line=="Y";
  auto snapshot=aoe2hd::ReadProductionQueue(selection,
   [&](uint32_t p,uint32_t off,uint32_t& out){return word(h.h,p,off,out);},
   [&](uint32_t p,void* data,size_t len){return read(h.h,p,data,len);});
  uint32_t pointer=snapshot.pointer;
  uint16_t groupCount=snapshot.groupCount,unitCount=snapshot.unitCount;
  bool readable=snapshot.valid()&&groupCount>0;
  auto result=snapshot.queue;
  if(!stable()){std::cerr<<"Selection changed while capturing\n";return 4;}
  bool equal=verified&&result.valid&&expected==result.expanded;
  valid+=result.valid;matched+=equal;
  rows<<stage<<",\""<<visual<<"\","<<verified<<','<<pointer<<','<<readable<<','
      <<groupCount<<','<<unitCount<<','<<result.valid<<",\""<<join(result.expanded)<<"\","
      <<equal<<",0\n";
  for(size_t i=0;i<result.groups.size();++i)
   groups<<stage<<','<<i<<','<<result.groups[i].type<<','<<result.groups[i].quantity<<"\n";
  std::cout<<" groups="<<groupCount<<" units="<<unitCount
           <<" structurally_valid="<<result.valid<<" visual_match="<<equal<<"\n";
 }
 std::cout<<"ProductionQueueStructureDiscovery v7 valid_stages="<<valid
          <<" matched_stages="<<matched<<"/8\n"
          <<"CSV: aoe2hd_production_queue_enumeration_v7.csv\n"
          <<"CSV: aoe2hd_production_queue_groups_v7.csv\n"
          <<"Unit queues only; research and existing map units are out of scope.\n";
 return 0;
}

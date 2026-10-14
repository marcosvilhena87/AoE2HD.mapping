#include "SnapshotDiff.h"
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <vector>
namespace {
struct Handle {HANDLE h; explicit Handle(HANDLE x):h(x){} ~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);} };
bool read(HANDLE h,uint32_t addr,void* out,size_t n){SIZE_T got=0;return ReadProcessMemory(h,reinterpret_cast<LPCVOID>(uintptr_t(addr)),out,n,&got)&&got==n;}
bool u32(HANDLE h,uint32_t p,uint32_t o,uint32_t& v){return p<=UINT32_MAX-o&&read(h,p+o,&v,4);}
uint32_t processId(){
 Handle h(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));
 if(h.h==INVALID_HANDLE_VALUE)return 0;
 PROCESSENTRY32W e{};e.dwSize=sizeof(e);
 if(!Process32FirstW(h.h,&e))return 0;
 do{if(_wcsicmp(e.szExeFile,L"AoK HD.exe")==0)return e.th32ProcessID;}while(Process32NextW(h.h,&e));
 return 0;
}
uint32_t moduleBase(uint32_t pid){
 Handle h(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid));
 if(h.h==INVALID_HANDLE_VALUE)return 0;
 MODULEENTRY32W e{};e.dwSize=sizeof(e);
 if(!Module32FirstW(h.h,&e))return 0;
 do{if(_wcsicmp(e.szModule,L"AoK HD.exe")==0){
 uintptr_t b=reinterpret_cast<uintptr_t>(e.modBaseAddr);return b<=UINT32_MAX?uint32_t(b):0;
 }}while(Module32NextW(h.h,&e));
 return 0;
}
bool capture(HANDLE h,uint32_t pid,uint32_t base,aoe2hd::Snapshot& s){
 uint32_t owner=0,world=0,vt=0,localIndex=0,begin=0,end=0,cap=0;
 if(base>UINT32_MAX-0x006F2C58||!u32(h,base,0x006F2C58,owner)||!owner||
    !u32(h,owner,0xC8,world)||!world||!u32(h,world,0,vt)||
    vt!=base+0x005D329C||!u32(h,world,0x174,localIndex)||
    !u32(h,world,0x184,begin)||!u32(h,world,0x188,end)||
    !u32(h,world,0x18C,cap)||!begin||end<begin||cap<end||
    (end-begin)%8||(cap-begin)%8||(end-begin)/8<2||
    (cap-begin)/8>9||localIndex==0||localIndex>=(end-begin)/8)return false;
 uint32_t player=0,ctrl=0,playerVt=0,back=0;
 if(!u32(h,begin,localIndex*8,player)||!u32(h,begin,localIndex*8+4,ctrl)||
    !player||ctrl>UINT32_MAX-12||player!=ctrl+12||
    !u32(h,player,0,playerVt)||playerVt!=base+0x005DCAE4||
    !u32(h,player,4,back)||back!=world)return false;
 s={};s.pid=pid;s.world=world;s.local=player;
 struct Root{const char* name;uint32_t ptr;};
 for(const auto& root:std::vector<Root>{{"World",world},{"LocalPlayer",player}}){
  for(uint32_t off=0x10;off+12<=0x800;off+=4){
   uint32_t words[3]{};
   if(root.ptr>UINT32_MAX-off-12||!read(h,root.ptr+off,words,12))continue;
   const uint32_t b=words[0],e=words[1],c=words[2];
   if(b<0x10000||b%4||e<=b||c<e||c-b>8192)continue;
   for(uint32_t stride:{4u,8u}){
    if((e-b)%stride||(c-b)%stride)continue;
    uint32_t count=(e-b)/stride;
    if(!count||count>512)continue;
    aoe2hd::Collection col;col.source=root.name;col.offset=off;
    col.begin=b;col.end=e;col.cap=c;col.stride=stride;
    bool valid=true;
    for(uint32_t i=0;i<count;++i){
     uint32_t v=0;
     if(!u32(h,b,i*stride,v)){valid=false;break;}
     col.values.push_back(v);
    }
    if(valid)s.collections.push_back(std::move(col));
   }
  }
 }
 uint32_t ownerAgain=0,worldAgain=0,playerAgain=0,b2=0,e2=0,c2=0;
 if(!u32(h,base,0x006F2C58,ownerAgain)||!u32(h,owner,0xC8,worldAgain)||
    !u32(h,begin,localIndex*8,playerAgain)||!u32(h,world,0x184,b2)||
    !u32(h,world,0x188,e2)||!u32(h,world,0x18C,c2)||
    ownerAgain!=owner||worldAgain!=world||playerAgain!=player||
    b2!=begin||e2!=end||c2!=cap)return false;
 return true;
}
void save(const std::string& path,const aoe2hd::Snapshot& s){
 std::ofstream out(path,std::ios::trunc);
 out<<"pid,world,local,source,offset,stride,begin,end,capacity_end,index,value\n";
 for(const auto& c:s.collections)for(size_t i=0;i<c.values.size();++i)
  out<<s.pid<<','<<s.world<<','<<s.local<<','<<c.source<<','<<c.offset<<','
     <<c.stride<<','<<c.begin<<','<<c.end<<','<<c.cap<<','<<i<<','<<c.values[i]<<'\n';
}
}
int main(){
 uint32_t pid=processId(),base=pid?moduleBase(pid):0;
 if(!base){std::cerr<<"AoK HD.exe nao encontrado.\n";return 1;}
 Handle h(OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,pid));
 if(!h.h){std::cerr<<"Falha ao abrir o processo.\n";return 1;}
 aoe2hd::Snapshot before,after;
 std::cout<<"SnapshotDiff experimental, somente leitura.\n"
          <<"Inicie uma partida estavel. Pressione ENTER para capturar A.\n";
 std::string input;std::getline(std::cin,input);
 if(!capture(h.h,pid,base,before)){std::cerr<<"Falha na captura A. Verifique se esta em partida.\n";return 2;}
 save("aoe2hd_snapshot_before.csv",before);
 std::cout<<"A: "<<before.collections.size()<<" interpretacoes candidatas.\n"
          <<"Crie EXATAMENTE um aldeao e aguarde a conclusao. Pressione ENTER para capturar B.\n";
 std::getline(std::cin,input);
 if(!capture(h.h,pid,base,after)){std::cerr<<"Falha na captura B.\n";return 2;}
 save("aoe2hd_snapshot_after.csv",after);
 if(before.pid!=after.pid||before.world!=after.world||before.local!=after.local){
  std::cerr<<"World ou jogador mudou; comparacao recusada. CSVs brutos preservados.\n";return 3;
 }
 auto changes=aoe2hd::Compare(before,after);
 std::ofstream csv("aoe2hd_snapshot_diff.csv",std::ios::trunc);
 if(!csv){std::cerr<<"Falha ao criar CSV.\n";return 4;}
 csv<<"source,offset,stride,kind,index,before,after\n";
 for(const auto& c:changes)csv<<c.source<<','<<c.offset<<','<<c.stride<<','
    <<c.kind<<','<<c.index<<','<<c.before<<','<<c.after<<'\n';
 std::cout<<"A="<<before.collections.size()<<" B="<<after.collections.size()
          <<" changes="<<changes.size()<<"\nCSV: aoe2hd_snapshot_diff.csv\n"
          <<"As alteracoes sao evidencias estruturais, nao unidades confirmadas.\n";
}

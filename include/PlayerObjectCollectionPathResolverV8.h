#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <cstdlib>
#include <cerrno>
namespace aoe2hd {
struct RootPathV8 { bool relative=false; bool world=false; uint32_t absolute=0; std::vector<uint32_t> offsets; };
inline bool ParseRootPathV8(const std::string& text,RootPathV8& output){
 RootPathV8 parsed;
 size_t pos=0;
 if(text.rfind("World+",0)==0){parsed.relative=true;parsed.world=true;pos=6;}
 else if(text.rfind("LocalPlayer+",0)==0){parsed.relative=true;pos=12;}
 else {
  if(text.empty())return false;
  char* end=nullptr;errno=0;
  unsigned long value=std::strtoul(text.c_str(),&end,16);
  if(errno||end!=text.c_str()+text.size()||value<0x10000||value>0xfffffd70ul||(value&3ul))return false;
  parsed.absolute=uint32_t(value);output=parsed;return true;
 }
 if(pos>=text.size())return false;
 while(pos<text.size()){
  if(parsed.offsets.size()>=3)return false;
  if(text.compare(pos,2,"0x")!=0&&text.compare(pos,2,"0X")!=0)return false;
  size_t start=pos+2;size_t end=text.find('+',start);
  std::string token=text.substr(start,end==std::string::npos?std::string::npos:end-start);
  if(token.empty()||token.size()>8)return false;
  for(char c:token)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return false;
  unsigned long off=std::strtoul(token.c_str(),nullptr,16);
  if(off>4092||(off&3ul))return false;
  parsed.offsets.push_back(uint32_t(off));
  if(end==std::string::npos)break;
  pos=end+1;
 }
 output=parsed;return true;
}
template<class ReadPointer>
bool ResolveRootPathV8(const RootPathV8& path,uint32_t world,uint32_t local,ReadPointer read,uint32_t& address){
 uint32_t ptr=path.relative?(path.world?world:local):path.absolute;
 if(ptr<0x10000||ptr>0xfffffd70u||(ptr&3u))return false;
 for(uint32_t offset:path.offsets){
  if(uint64_t(ptr)+offset>0xffffffffull)return false;
  uint32_t next=0;if(!read(uint32_t(ptr+offset),next)||next<0x10000||next>0xfffffd70u||(next&3u))return false;
  ptr=next;
 }
 address=ptr;return true;
}
} // namespace aoe2hd

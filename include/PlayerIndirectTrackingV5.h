#pragma once
#include <cstdint>
#include <vector>
#include <algorithm>
namespace aoe2hd {
struct ContentSnapshotV5 {uint32_t address=0;std::vector<uint8_t> bytes;bool readable=false;};
template<class Read>
ContentSnapshotV5 SnapshotContentV5(Read&& read,uint32_t address,uint32_t size=128){
 ContentSnapshotV5 result;result.address=address;
 if(address<0x10000||address>UINT32_MAX-size||size==0||size>512)return result;
 result.bytes.resize(size);
 result.readable=read(address,result.bytes.data(),size);
 if(!result.readable)result.bytes.clear();
 return result;
}
inline uint32_t ChangedBytesV5(const ContentSnapshotV5& a,const ContentSnapshotV5& b){
 if(!a.readable||!b.readable||a.address!=b.address||a.bytes.size()!=b.bytes.size())return 0;
 uint32_t count=0;
 for(size_t i=0;i<a.bytes.size();++i)count+=(a.bytes[i]!=b.bytes[i]);
 return count;
}
}
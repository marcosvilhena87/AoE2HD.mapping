#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
namespace aoe2hd {
struct RepeatedEventField {
 uint32_t values[5]{};
 bool control1=false,creation1=false,control2=false,creation2=false;
 bool cleanRepeat=false,incrementOne=false;
};
inline RepeatedEventField AnalyzeRepeatedEvent(const uint32_t (&v)[5]){
 RepeatedEventField f;
 for(int i=0;i<5;++i)f.values[i]=v[i];
 f.control1=v[0]!=v[1];f.creation1=v[1]!=v[2];
 f.control2=v[2]!=v[3];f.creation2=v[3]!=v[4];
 f.cleanRepeat=!f.control1&&!f.control2&&f.creation1&&f.creation2;
 f.incrementOne=f.cleanRepeat&&uint32_t(v[2]-v[1])==1u&&uint32_t(v[4]-v[3])==1u;
 return f;
}
inline uint32_t ReadWordEvent(const std::vector<uint8_t>& bytes,size_t at){
 uint32_t n=0;if(at+4<=bytes.size())std::memcpy(&n,bytes.data()+at,4);return n;
}
}
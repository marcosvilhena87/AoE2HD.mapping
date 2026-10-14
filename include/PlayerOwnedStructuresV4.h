#pragma once
#include <cstdint>
#include <vector>
namespace aoe2hd {
struct OwnedField {uint32_t player=0,sub=0,offset=0,before=0,after=0;};
inline bool UsefulOwnedPointer(uint32_t value,uint32_t world,uint32_t a,uint32_t b){
 return value>=0x10000 && !(value&3u) && value!=world&&value!=a&&value!=b;
}
inline bool Changed(const OwnedField& f){return f.before!=f.after;}
template<class Reader>
std::vector<uint32_t> ReadWords(Reader&& reader,uint32_t base,uint32_t bytes=0x400){
 std::vector<uint32_t> words(bytes/4,0);
 for(uint32_t i=0;i<bytes/4;++i)reader(base+i*4,&words[i],4);
 return words;
}
}
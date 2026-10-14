#pragma once
#include <cstdint>
namespace aoe2hd {
struct SplitCounterV3 {uint16_t low=0,high=0;};
inline SplitCounterV3 SplitCounter(uint32_t word){
 return {uint16_t(word&0xffffu),uint16_t(word>>16)};
}
inline int Delta16(uint16_t previous,uint16_t current){
 return int(current)-int(previous);
}
}
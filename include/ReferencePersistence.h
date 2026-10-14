#pragma once
#include <cstdint>
namespace aoe2hd {
inline bool IsPersistentReference(bool readable, uint32_t observed, uint32_t original){
 return readable && original >= 0x10000 && observed == original;
}
inline bool IsSelectionSlot(uint32_t address,uint32_t local){
 return local <= UINT32_MAX-0x160 &&
 (address==local+0x15c || address==local+0x160);
}
}
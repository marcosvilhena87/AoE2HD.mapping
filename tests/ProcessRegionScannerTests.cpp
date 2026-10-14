#include "ProcessRegionScanner.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#include <iostream>
int main(){
 std::vector<uint8_t> bytes(32,0);
 uint32_t anchor=0x14722150, left=123,right=456;
 std::memcpy(bytes.data()+4,&left,4);
 std::memcpy(bytes.data()+8,&anchor,4);
 std::memcpy(bytes.data()+12,&right,4);
 std::memcpy(bytes.data()+20,&anchor,4);
 auto found=aoe2hd::SearchPointerChunk(0x20000,bytes.data(),bytes.size(),anchor);
 assert(found.size()==2);
 assert(found[0].address==0x20008&&found[0].before==left&&found[0].after==right);
 assert(found[1].address==0x20014);
 assert(aoe2hd::SearchPointerChunk(0x20000,bytes.data(),bytes.size(),anchor,1).size()==1);
 assert(aoe2hd::SearchPointerChunk(0x20001,bytes.data(),bytes.size(),anchor).empty());
 assert(aoe2hd::SearchPointerChunk(0x20000,bytes.data(),bytes.size(),0).empty());
 assert(aoe2hd::SearchPointerChunk(0x20000,nullptr,bytes.size(),anchor).empty());
 std::cout<<"ProcessRegionScannerTests OK\n";
}

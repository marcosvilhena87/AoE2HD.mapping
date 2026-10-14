#include "DualAnchorDiscovery.h"
#include <cassert>
#include <cstring>
#include <vector>
int main(){
 const uint32_t a=0x14722150,b=0x109fc348;
 std::vector<uint8_t> bytes(64,0);
 std::memcpy(bytes.data()+8,&a,4);
 std::memcpy(bytes.data()+24,&b,4);
 std::memcpy(bytes.data()+40,&a,4);
 auto hits=aoe2hd::SearchTwoAnchors(0x20000,bytes.data(),bytes.size(),a,b,10);
 assert(hits.size()==3);
 auto pairs=aoe2hd::FindNearbyPairs(hits,a,b,20,10);
 assert(pairs.size()==2);
 assert(pairs[0].first==0x20008&&pairs[0].second==0x20018);
 assert(aoe2hd::FindNearbyPairs(hits,a,b,8,10).empty());
 assert(aoe2hd::SearchTwoAnchors(0x20000,bytes.data(),bytes.size(),a,a,10).empty());
 assert(aoe2hd::SearchTwoAnchors(0x20000,bytes.data(),bytes.size(),a,b,1).size()==1);
 assert(aoe2hd::SearchTwoAnchors(0x20000,nullptr,0,a,b,10).empty());
 return 0;
}
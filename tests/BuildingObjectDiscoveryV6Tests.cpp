#include "BuildingObjectDiscoveryV6.h"
#include <cassert>
int main(){
 std::vector<uint8_t> b(32,0);
 b[0]=5;b[4]=7;b[8]=5;b[12]=7;b[16]=5;b[20]=7;
 auto s=aoe2hd::ScoreStridesV6(b,8,8,4);
 assert(s.size()==1&&s[0].pairs==6&&s[0].nonzeroPairs==4);
 assert(s[0].equalNonzero==4&&s[0].rareMatches==4&&s[0].commonMatches==0);
 assert(aoe2hd::ScoreStridesV6(b,0,8,0).empty());
 auto zero=aoe2hd::ScoreStridesV6(std::vector<uint8_t>(16,0),8,8,4);
 assert(zero.size()==1&&zero[0].equalZero==2&&zero[0].rate()==0);
}

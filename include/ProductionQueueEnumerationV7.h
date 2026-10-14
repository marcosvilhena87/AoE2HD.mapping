#pragma once
#include "ProductionQueueEntriesV6.h"
#include <cstdint>
#include <vector>
namespace aoe2hd {
struct QueueEnumerationV7 {
 bool valid=false;
 std::vector<QueueEntryV6> groups;
 std::vector<uint16_t> expanded;
};
inline QueueEnumerationV7 EnumerateQueueV7(const std::vector<uint8_t>& bytes,
 uint16_t groupCount,uint16_t totalCount,size_t maxGroups=128,size_t maxUnits=1000){
 QueueEnumerationV7 out;
 if(groupCount>maxGroups||totalCount>maxUnits)return out;
 if(groupCount==0){out.valid=totalCount==0;return out;}
 if(groupCount>bytes.size()/4||totalCount<groupCount)return out;
 for(size_t i=0;i<groupCount;++i){
  auto e=DecodeQueueEntryV6(bytes.data()+4*i);
  if(e.quantity==0||e.quantity>totalCount-out.expanded.size())return QueueEnumerationV7{};
  // Adjacent equal types should be aggregated under this experimental model.
  if(!out.groups.empty()&&out.groups.back().type==e.type)return QueueEnumerationV7{};
  out.groups.push_back(e);
  out.expanded.insert(out.expanded.end(),e.quantity,e.type);
 }
 out.valid=out.expanded.size()==totalCount;
 if(!out.valid)return QueueEnumerationV7{};
 return out;
}
} // namespace aoe2hd

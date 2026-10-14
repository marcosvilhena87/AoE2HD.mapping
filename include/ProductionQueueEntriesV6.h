#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
namespace aoe2hd {
struct QueueEntryV6 {uint16_t type=0,quantity=0;};
inline QueueEntryV6 DecodeQueueEntryV6(const uint8_t* p){
 uint16_t a=0,b=0;std::memcpy(&a,p,2);std::memcpy(&b,p+2,2);return {a,b};
}
inline std::vector<QueueEntryV6> ReadQueueEntriesV6(const std::vector<uint8_t>& bytes,size_t limit=16){
 std::vector<QueueEntryV6> result;
 for(size_t i=0;i<limit&&4*i+4<=bytes.size();++i)
  result.push_back(DecodeQueueEntryV6(bytes.data()+4*i));
 return result;
}
inline bool CompareQueueV6(const std::vector<QueueEntryV6>& entries,const std::vector<uint16_t>& types,size_t start,bool aggregate){
 size_t j=0;
 for(size_t i=0;i<types.size();){
  if(start+j>=entries.size())return false;
  size_t run=1;
  if(aggregate)while(i+run<types.size()&&types[i+run]==types[i])++run;
  if(entries[start+j].type!=types[i]||entries[start+j].quantity!=run)return false;
  i+=run;++j;
 }
 return true;
}
} // namespace aoe2hd

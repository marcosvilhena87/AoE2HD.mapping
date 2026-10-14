#pragma once
#include <cstdint>
#include <vector>
#include <string>
namespace aoe2hd {
struct QueueObservationV5 {std::string stage;int queue=-1;int completed=-1;bool verified=false;uint16_t queueField=0,completedField=0;};
inline bool QueueMatchesV5(const std::vector<QueueObservationV5>& v){
 if(v.size()<3)return false;
 for(size_t i=1;i<v.size();++i){
  if(!v[i].verified||!v[i-1].verified||v[i].queue<0||v[i-1].queue<0)return false;
  if(int(v[i].queueField)-int(v[i-1].queueField)!=v[i].queue-v[i-1].queue)return false;
 }
 return true;
}
inline bool CompletionMatchesV5(const std::vector<QueueObservationV5>& v){
 if(v.size()<3)return false;
 for(size_t i=1;i<v.size();++i){
  if(!v[i].verified||!v[i-1].verified||v[i].completed<0||v[i-1].completed<0)return false;
  if(int(v[i].completedField)-int(v[i-1].completedField)!=v[i].completed-v[i-1].completed)return false;
 }
 return true;
}
}
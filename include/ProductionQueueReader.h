#pragma once
#include "ProductionQueueEnumerationV7.h"
#include <cstdint>
#include <vector>
#include <utility>
namespace aoe2hd {
enum class QueueReadStatus {Ok, InvalidPointer, ReadFailure, InvalidLayout, ChangedDuringRead};
struct ProductionQueueSnapshot {
 QueueReadStatus status=QueueReadStatus::InvalidLayout;
 uint32_t pointer=0;
 uint16_t groupCount=0, unitCount=0;
 QueueEnumerationV7 queue;
 bool valid() const {return status==QueueReadStatus::Ok&&queue.valid;}
};
template<class ReadWord,class ReadBytes>
ProductionQueueSnapshot ReadProductionQueue(uint32_t building,ReadWord word,ReadBytes bytes){
 ProductionQueueSnapshot r;uint32_t ptr=0,w218=0,w21c=0;
 if(!word(building,0x214,ptr)||!word(building,0x218,w218)||!word(building,0x21c,w21c)){
  r.status=QueueReadStatus::ReadFailure;return r;
 }
 r.pointer=ptr;r.groupCount=uint16_t(w218>>16);r.unitCount=uint16_t(w21c);
 if(r.groupCount==0&&r.unitCount==0){
  r.queue=EnumerateQueueV7({},0,0);r.status=QueueReadStatus::Ok;return r;
 }
 if(r.groupCount==0||r.groupCount>128||r.unitCount<r.groupCount||r.unitCount>1000){
  r.status=QueueReadStatus::InvalidLayout;return r;
 }
 if(ptr<0x10000||(ptr&3u)||ptr>UINT32_MAX-4u*r.groupCount){
  r.status=QueueReadStatus::InvalidPointer;return r;
 }
 std::vector<uint8_t> data(4u*r.groupCount);
 if(!bytes(ptr,data.data(),data.size())){r.status=QueueReadStatus::ReadFailure;return r;}
 uint32_t ptr2=0,w218b=0,w21cb=0;
 if(!word(building,0x214,ptr2)||!word(building,0x218,w218b)||!word(building,0x21c,w21cb)){
  r.status=QueueReadStatus::ReadFailure;return r;
 }
 if(ptr2!=ptr||w218b!=w218||w21cb!=w21c){
  r.status=QueueReadStatus::ChangedDuringRead;return r;
 }
 r.queue=EnumerateQueueV7(data,r.groupCount,r.unitCount);
 r.status=r.queue.valid?QueueReadStatus::Ok:QueueReadStatus::InvalidLayout;
 return r;
}
} // namespace aoe2hd

#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
namespace aoe2hd {
struct PointerSnapshotV8 {
 bool slotRead=false,targetRead=false;
 uint32_t pointer=0,id=0,firstWord=0,thirdWord=0;
};
struct PointerTransitionV8 {
 bool complete=false,controlStable=false,interventionChanged=false;
 bool controlChanged=false,postTargetReadable=false,idEqualsKnown=false;
 bool candidate=false;
};
inline PointerTransitionV8 ClassifyPointerV8(
 const PointerSnapshotV8& a,const PointerSnapshotV8& control,
 const PointerSnapshotV8& after,const std::vector<uint32_t>& knownIds){
 PointerTransitionV8 r;
 r.complete=a.slotRead&&control.slotRead&&after.slotRead;
 if(!r.complete)return r;
 r.controlStable=a.pointer==control.pointer;
 r.controlChanged=!r.controlStable;
 r.interventionChanged=control.pointer!=after.pointer;
 r.postTargetReadable=after.targetRead;
 for(uint32_t id:knownIds)if(after.targetRead&&after.id==id)r.idEqualsKnown=true;
 r.candidate=r.controlStable&&r.interventionChanged&&r.postTargetReadable;
 return r;
}
} // namespace aoe2hd

#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
namespace aoe2hd {
struct PlayerEntryEvidence {
 uint32_t slot=0,index=0,field=0,raw=0,indirect=0,object=0,id=0;
 float hp=0;bool direct=false,viaIndirect=false,plausible=false;
};
template<class Reader>
PlayerEntryEvidence InspectPlayerEntry(Reader&& read,uint32_t slot,uint32_t index,
 uint32_t field,uint32_t anchorA,uint32_t anchorB){
 PlayerEntryEvidence e;e.slot=slot;e.index=index;e.field=field;
 if(slot>UINT32_MAX-field||!read(slot+field,&e.raw,4))return e;
 e.direct=(e.raw==anchorA||e.raw==anchorB);
 auto inspect=[&](uint32_t obj)->bool{
  if(obj<0x10000||(obj&3u)||obj>UINT32_MAX-0x38)return false;
  uint32_t id=0,bits=0;
  if(!read(obj+8,&id,4)||!read(obj+0x34,&bits,4))return false;
  float hp=0;std::memcpy(&hp,&bits,4);
  if(!std::isfinite(hp)||hp<0||hp>100000||id==0||id>1000000)return false;
  e.object=obj;e.id=id;e.hp=hp;e.plausible=true;return true;
 };
 if(inspect(e.raw))return e;
 if(e.raw<0x10000||(e.raw&3u)||e.raw>UINT32_MAX-4)return e;
 uint32_t second=0;
 if(read(e.raw,&second,4)){
  e.indirect=second;
  if(inspect(second))e.viaIndirect=true;
 }
 return e;
}
}
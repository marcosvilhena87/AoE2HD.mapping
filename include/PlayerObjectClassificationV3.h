#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
namespace aoe2hd {
struct ObjectFingerprint {uint32_t address=0,vt=0,id=0,owner=0;float hp=0;bool readable=false,hpSane=false;};
template<class Read>
ObjectFingerprint FingerprintObject(Read&& read,uint32_t address){
 ObjectFingerprint f;f.address=address;
 if(address<0x10000||(address&3u)||address>UINT32_MAX-0x38)return f;
 uint32_t bits=0;
 if(!read(address,&f.vt,4)||!read(address+8,&f.id,4)||
    !read(address+0x10,&f.owner,4)||!read(address+0x34,&bits,4))return f;
 std::memcpy(&f.hp,&bits,4);f.readable=true;
 f.hpSane=std::isfinite(f.hp)&&f.hp>=1&&f.hp<=100000;
 return f;
}
inline const char* ClassifyObject(const ObjectFingerprint& f,
 const ObjectFingerprint& a,const ObjectFingerprint& b){
 if(!f.readable)return "unreadable";
 if(f.address==a.address||f.address==b.address)return "known_anchor";
 if(f.vt&&f.vt==a.vt&&f.vt==b.vt&&f.hpSane)return "anchor_vtable_match";
 if(f.vt&&f.vt==a.vt&&f.hpSane)return "A_vtable_match";
 if(f.vt&&f.vt==b.vt&&f.hpSane)return "B_vtable_match";
 if(!f.hpSane)return "non_unit_like_hp";
 return "unknown_plausible";
}
}
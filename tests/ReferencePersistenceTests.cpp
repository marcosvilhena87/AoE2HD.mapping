#include "ReferencePersistence.h"
#include <cassert>
int main(){
 using namespace aoe2hd;
 assert(IsPersistentReference(true,0x14722150,0x14722150));
 assert(!IsPersistentReference(true,0x14722154,0x14722150));
 assert(!IsPersistentReference(false,0x14722150,0x14722150));
 assert(!IsPersistentReference(true,0,0));
 assert(IsSelectionSlot(0x18B63388,0x18B6322C));
 assert(IsSelectionSlot(0x18B6338C,0x18B6322C));
 assert(!IsSelectionSlot(0x18B63390,0x18B6322C));
 assert(!IsSelectionSlot(0xffffffff,0xffffffff));
}
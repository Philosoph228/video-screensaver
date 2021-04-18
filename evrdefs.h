#ifndef EVRDEFS_H
#define EVRDEFS_H

#include <uuids.h>
#include "mfisvc.h"

#ifndef OUR_GUID_ENTRY
#define OUR_GUID_ENTRY(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8);
#endif

OUR_GUID_ENTRY(CLSID_EnhancedVideoRenderer,0xFA10746C,0x9B63,0x4B6C,0xBC,0x49,0xFC,0x30,0x0E,0xA5,0xF2,0x56)

#endif  /* EVRDEFS_H */

#ifndef _XBOX360_H_
#define _XBOX360_H_

#include <gctypes.h>

#ifdef __cplusplus
extern "C" {
#endif

bool XBOX360_ScanPads();
u32 XBOX360_ButtonsHeld(int chan);
s32 XBOX360_Endpoint();

#ifdef __cplusplus
}
#endif

#endif

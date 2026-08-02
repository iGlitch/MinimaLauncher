#ifndef __PATCHCODE_H__
#define __PATCHCODE_H__

#include <gctypes.h>

/*
0 No Hook
1 VBI
2 KPAD read
3 Joypad Hook
4 GXDraw Hook
5 GXFlush Hook
6 OSSleepThread Hook
7 AXNextFrame Hook
*/
#define HOOKTYPE 1

// Globals
extern const u32 hookdata[4];

// Function prototypes
bool dogamehooks(void *addr, u32 len);
void Https_Patch(void *addr, u32 len);

#endif // __PATCHCODE_H__

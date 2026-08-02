#ifndef _DISC_H_
#define _DISC_H_

#include <gccore.h>

s32 Disc_Open(void);
s32 Disc_FindPartition(u32 *outbuf);
void Disc_SetLowMemPre(void);
void Disc_SetLowMem(u32 IOS);
void Disc_SetTime(void);

GXRModeObj *Disc_SelectVMode(u32 *rmode_reg);
void Disc_SetVMode(GXRModeObj *rmode, u32 rmode_reg);

#endif

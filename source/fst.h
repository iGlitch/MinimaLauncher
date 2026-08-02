#ifndef __FST_H__
#define __FST_H__

#include <gctypes.h>

int app_gameconfig_load(const char *discid, u8 *tempgameconf, u32 tempgameconfsize);
void ocarina_set_codes(void *list, u8 *listend, u8 *cheats, u32 cheatSize);
void ocarina_do_code(void);

#endif

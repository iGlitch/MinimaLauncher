#ifndef __FST_H__
#define __FST_H__

#ifdef __cplusplus
extern "C" {
#endif

extern u8 debuggerselect;

#define MAX_GCT_SIZE 2056

int app_gameconfig_load(const char *discid, u8 *tempgameconf, u32 tempgameconfsize);
void app_gameconfig_set(u32 *gameconfig, u32 tempgameconfsize);
void ocarina_set_codes(void *list, u8 *listend, u8 *cheats, u32 cheatSize);
int ocarina_do_code();

#ifdef __cplusplus
}
#endif

#endif

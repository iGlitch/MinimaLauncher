#ifndef __PATCHCODE_H__
#define __PATCHCODE_H__

#ifdef __cplusplus
extern "C" {
#endif
// Globals
extern u32 hooktype;
extern u8 configbytes[2];

// Function prototypes
bool dogamehooks(void *addr, u32 len, bool channel);
void langpatcher(void *addr, u32 len);
void vidolpatcher(void *addr, u32 len);
void PatchVideoSneek(void *addr, u32 len);
void PatchCountryStrings(void *Address, int Size);
void PatchAspectRatio(void *addr, u32 len, u8 aspect);
bool PatchReturnTo(void *Address, int Size, u32 id);
void Patch_fwrite(void *Address, int Size);
s32 BlockIOSReload(void);
void PatchRegion(void *Address, int Size);
void Https_Patch(void *addr, u32 len);

#ifdef __cplusplus
}
#endif

#endif // __PATCHCODE_H__

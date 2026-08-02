#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <ogc/machine/processor.h>
#include "apploader.h"
#include "memory.h"
#include "disc.h"
#include "wdvd.h"
#include "defines.h"
#include "fst.h"

volatile u32 AppEntrypoint = 0;

extern void __exception_closeall(void) __attribute__((weak));

static u8 *load_file(const char *path, size_t *size)
{
	FILE *f = fopen(path, "rb");
	if(f == NULL)
		return NULL;

	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	rewind(f);

	u8 *buf = NULL;
	if(len > 0)
	{
		buf = malloc(len + 1);
		if(buf != NULL && fread(buf, len, 1, f) == 1)
			*size = len;
		else
		{
			free(buf);
			buf = NULL;
		}
	}
	fclose(f);
	return buf;
}

int main(void)
{
	if((*(vu32*)0xCD8005A0 >> 16) == 0xCAFE) // Wii U
		write32(0xd8006a0, 0x30000004), mask32(0xd8006a8, 0, 2);

	VIDEO_Init();

	/* Setup Low Memory */
	Disc_SetLowMemPre();

	/* Get Disc Status */
	WDVD_Init();
	u32 disc_check = 0;
	WDVD_GetCoverStatus(&disc_check);
	if(disc_check & 0x2)
	{
		/* Open up Disc */
		Disc_Open();

		/* read in cheats */
		WDVD_ReadDiskId((u8*)Disc_ID);
		const DISC_INTERFACE *sd = &__io_wiisd;
		sd->startup();
		fatMountSimple("sd", sd);

		/* gameconfig */
		size_t fsize = 0;
		u8 *gameconfig = load_file("sd:/Minusery/gc.txt", &fsize);
		if(gameconfig != NULL)
		{
			app_gameconfig_load((char*)Disc_ID, gameconfig, fsize);
			free(gameconfig);
		}

		/* gct */
		char gamepath[] = "sd:/Minusery/______.gct";
		memcpy(gamepath + 13, (char*)Disc_ID, 6);
		u8 *cheats = load_file(gamepath, &fsize);
		if(cheats != NULL)
		{
			ocarina_set_codes((void*)0x800022A8, (u8*)0x80003000, cheats, fsize);
			free(cheats);
		}

		fatUnmount("sd");
		sd->shutdown();

		/* Find our Partition */
		u32 GameIOS = 58;
		u32 offset = 0;
		Disc_FindPartition(&offset);
		WDVD_OpenPartition(offset, &GameIOS);
		WDVD_Close();
		IOS_ReloadIOS(GameIOS);

		/* Re-Init after IOS Reload */
		WDVD_Init();
		WDVD_ReadDiskId((u8*)Disc_ID);
		WDVD_OpenPartition(offset, &GameIOS);

		/* Run Apploader */
		AppEntrypoint = Apploader_Run();
		WDVD_Close();

		/* Setup Low Memory */
		Disc_SetLowMem(GameIOS);

		/* Set an appropriate video mode */
		u32 vmode_reg = 0;
		GXRModeObj *vmode = Disc_SelectVMode(&vmode_reg);
		Disc_SetVMode(vmode, vmode_reg);

		/* Set time */
		Disc_SetTime();

		/* Shutdown IOS subsystems */
		u32 level = IRQ_Disable();
		__IOS_ShutdownSubsystems();
		if(__exception_closeall)
			__exception_closeall();

		/* Originally from tueidj - taken from NeoGamma (thx) */
		*(vu32*)0xCC003024 = 1;

		/* Boot */
		asm volatile (
			"lis %r3, AppEntrypoint@h\n"
			"ori %r3, %r3, AppEntrypoint@l\n"
			"lwz %r3, 0(%r3)\n"
			"mtlr %r3\n"
			"lis %r3, 0x8000\n"
			"ori %r3, %r3, 0x18A8\n"
			"mtctr %r3\n"
			"bctr\n"
			);

		/* Fail */
		IRQ_Restore(level);
	}

	/* Fail, init chan launching */
	WII_Initialize();

	/* goto HBC */
	WII_LaunchTitle(HBC_LULZ);
	WII_LaunchTitle(HBC_108);
	WII_LaunchTitle(HBC_JODI);
	WII_LaunchTitle(HBC_HAXX);

	/* Fail, goto System Menu */
	WII_LaunchTitle(SYSTEM_MENU);
	return 0;
}

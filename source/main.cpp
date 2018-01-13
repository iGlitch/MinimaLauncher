/****************************************************************************
 * Copyright (C) 2012-2014
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <unistd.h>
#include <malloc.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <ogc/machine/processor.h>
#include <wiiuse/wpad.h>
#include "apploader.h"
#include "memory.h"
#include "utils.h"
#include "disc.h"
#include "wdvd.h"
#include "gecko.h"
#include "defines.h"
#include "gc.hpp"
#include "fst.h"
#include "gameconfig_ssbb.h"
#include "gameconfig_kirby.h"
#include "patchcode.h"

/* Boot Variables */
u32 GameIOS = 0;
u32 vmode_reg = 0;
GXRModeObj *vmode = NULL;

u32 AppEntrypoint = 0;

extern "C" {
extern void __exception_closeall();
}

int main()
{
	if ( (*(u32*)(0xCD8005A0) >> 16 ) == 0xCAFE ) // Wii U
	{
		/* vWii widescreen patch by tueidj */
		write32(0xd8006a0, 0x30000004), mask32(0xd8006a8, 0, 2);
	}

	InitGecko();
	gprintf("MinimaLauncher v1.2\n");
	VIDEO_Init();
	WPAD_Init();
	PAD_Init();

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
		if(Disc_IsGC() == 0)
		{
			WII_Initialize();
			gprintf("Booting GC Disc\n");
			/* Set Video Mode and Language */
			u8 region = ((u8*)Disc_ID)[3];
			u8 videoMode = 1; //PAL 576i
			if(region == 'E' || region == 'J')
				videoMode = 2; //NTSC 480i
			GC_SetVideoMode(videoMode);
			GC_SetLanguage();
			/* if DM is installed */
			DML_New_SetBootDiscOption();
			DML_New_WriteOptions();
			/* Set time */
			Disc_SetTime();
			/* NTSC-J Patch by FIX94 */
			if(region == 'J')
				*HW_PPCSPEED = 0x0002A9E0;
			/* Boot BC */
			WII_LaunchTitle(GC_BC);
		}
		else if(Disc_IsWii() == 0)
		{
			FILE *f = NULL;
			size_t fsize = 0;
			/* read in cheats */
			WDVD_ReadDiskId((u8*)Disc_ID);
			const DISC_INTERFACE *sd = &__io_wiisd;
			sd->startup();
			if(sd->isInserted())
				gprintf("sd inserted!\n");
			fatMountSimple("sd", sd);
			/* gameconfig */
			if((*(u32*)Disc_ID & 0xFFFFFF00) == 0x52534200)//rsb?01 brawl
				app_gameconfig_load((char*)Disc_ID, (u8*)gameconfig_ssbb, gameconfig_ssbb_size);
			else if(*(u32*)Disc_ID == 0x53554B45)//suke01 kirby
				app_gameconfig_load((char*)Disc_ID, (u8*)gameconfig_kirby, gameconfig_kirby_size);			
			else
			{
				f = fopen("sd:/gameconfig.txt", "rb");
				if(f != NULL)
				{
					fseek(f, 0, SEEK_END);
					fsize = ftell(f);
					rewind(f);
					u8 *gameconfig = (u8*)malloc(fsize);
					fread(gameconfig, fsize, 1, f);
					fclose(f);
					app_gameconfig_load((char*)Disc_ID, gameconfig, fsize);
					free(gameconfig);
				}
			}
			/* gct */
			char gamepath[22];
			if((*(u32*)Disc_ID & 0xFFFFFF00) == 0x52534200)//rsb?01 brawl
			{
				PAD_ScanPads();
				u32 pad_down = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);
				WPAD_ScanPads();
				u32 wpad_down = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);
				if ((wpad_down & WPAD_BUTTON_LEFT) || (pad_down & PAD_BUTTON_LEFT))
					sprintf(gamepath, "sd:/codes/ProjectM.gct");
				else if ((wpad_down & WPAD_BUTTON_RIGHT) || (pad_down & PAD_BUTTON_RIGHT))
					sprintf(gamepath, "sd:/codes/Minus.gct");
				else
					sprintf(gamepath, "sd:/codes/%.6s.gct", (char*)Disc_ID);
			}
			else
			{
				sprintf(gamepath, "sd:/codes/%.6s.gct", (char*)Disc_ID);
			}
			gprintf("%s\n", gamepath);
			f = fopen(gamepath, "rb");
			if(f != NULL)
			{
				gprintf("Opened gct\n");
				fseek(f, 0, SEEK_END);
				fsize = ftell(f);
				rewind(f);
				u8 *cheats = (u8*)malloc(fsize);
				fread(cheats, fsize, 1, f);
				fclose(f);
				ocarina_set_codes((void*)0x800022A8, (u8*)0x80003000, cheats, fsize);
				free(cheats);
			}
			fatUnmount("sd");
			sd->shutdown();
			/* Find our Partition */
			u32 offset = 0;
			Disc_FindPartition(&offset);
			WDVD_OpenPartition(offset, &GameIOS);
			gprintf("Using IOS: %i\n", GameIOS);
			WDVD_Close();
			if((*(u32*)Disc_ID & 0xFFFFFF00) == 0x52534200)
				IOS_ReloadIOS(58); //for SDHC support
			else
				IOS_ReloadIOS(GameIOS); //supports all other games
			/* Re-Init after IOS Reload */
			WDVD_Init();
			WDVD_ReadDiskId((u8*)Disc_ID);
			WDVD_OpenPartition(offset, &GameIOS);
			/* Run Apploader */
			AppEntrypoint = Apploader_Run();
			gprintf("Entrypoint: %08x\n", AppEntrypoint);
			WDVD_Close();
			/* Setup Low Memory */
			Disc_SetLowMem(GameIOS);
			/* Set an appropriate video mode */
			vmode = Disc_SelectVMode(&vmode_reg);
			Disc_SetVMode(vmode, vmode_reg);
			/* Set time */
			Disc_SetTime();
			/* Shutdown IOS subsystems */
			u32 level = IRQ_Disable();
			__IOS_ShutdownSubsystems();
			__exception_closeall();
			/* Originally from tueidj - taken from NeoGamma (thx) */
			*(vu32*)0xCC003024 = 1;
			/* Boot */
            if(hooktype == 0)
            {
                asm volatile (
                    "lis %r3, AppEntrypoint@h\n"
                    "ori %r3, %r3, AppEntrypoint@l\n"
                    "lwz %r3, 0(%r3)\n"
                    "mtlr %r3\n"
                    "blr\n"
                );
            }
            else
			{
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
			}
			/* Fail */
			IRQ_Restore(level);
		}
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

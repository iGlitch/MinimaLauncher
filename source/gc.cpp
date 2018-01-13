/****************************************************************************
 * Copyright (C) 2012-2013 FIX94
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
#include <gccore.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ogc/machine/processor.h>
#include <ogc/lwp_threads.h>
#include "gc.hpp"
#include "memory.h"

// DIOS-MIOS
DML_CFG DMLCfg;

void DML_New_SetBootDiscOption()
{
	memset(&DMLCfg, 0, sizeof(DML_CFG));

	DMLCfg.Magicbytes = 0xD1050CF6;
	DMLCfg.CfgVersion = 0x00000002;

	DMLCfg.VideoMode |= DML_VID_DML_AUTO;
	DMLCfg.Config |= DML_CFG_BOOT_DISC;
}

void DML_New_WriteOptions()
{
	//Write options into memory
	memcpy((void *)0x80001700, &DMLCfg, sizeof(DML_CFG));
	DCFlushRange((void *)(0x80001700), sizeof(DML_CFG));

	//DML v1.2+
	memcpy((void *)0x81200000, &DMLCfg, sizeof(DML_CFG));
	DCFlushRange((void *)(0x81200000), sizeof(DML_CFG));
}

// General
#define SRAM_ENGLISH 0
#define SRAM_GERMAN 1
#define SRAM_FRENCH 2
#define SRAM_SPANISH 3
#define SRAM_ITALIAN 4
#define SRAM_DUTCH 5

extern "C" {
syssram* __SYS_LockSram();
u32 __SYS_UnlockSram(u32 write);
u32 __SYS_SyncSram(void);
}

void GC_SetVideoMode(u8 videomode)
{
	syssram *sram = __SYS_LockSram();
	GXRModeObj *vmode = VIDEO_GetPreferredMode(0);
	int vmode_reg = 0;

	if(VIDEO_HaveComponentCable() && (CONF_GetProgressiveScan() > 0))
		sram->flags |= 0x80; //set progressive flag
	else
		sram->flags &= 0x7F; //clear progressive flag

	if(videomode == 1) //pal
	{
		vmode_reg = 1;
		sram->flags |= 0x01; // Set bit 0 to set the video mode to PAL
		sram->ntd |= 0x40; //set pal60 flag
	}
	else //ntsc
	{
		sram->flags &= 0xFE; // Clear bit 0 to set the video mode to NTSC
		sram->ntd &= 0xBF; //clear pal60 flag
	}

	if(videomode == 1)
		vmode = &TVPal528IntDf;
	else
		vmode = &TVNtsc480IntDf;

	__SYS_UnlockSram(1); // 1 -> write changes
	while(!__SYS_SyncSram());
	sram = NULL;

	/* Set video mode register */
	*Video_Mode = vmode_reg;
	DCFlushRange((void*)Video_Mode, 4);

	/* Set video mode */
	if(vmode != 0)
		VIDEO_Configure(vmode);

	/* Setup video */
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(vmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
	else while(VIDEO_GetNextField())
		VIDEO_WaitVSync();

	/* Set black and flush */
	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(vmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
	else while(VIDEO_GetNextField())
		VIDEO_WaitVSync();
}

u8 get_wii_language()
{
	switch (CONF_GetLanguage())
	{
		case CONF_LANG_GERMAN:
			return SRAM_GERMAN;
		case CONF_LANG_FRENCH:
			return SRAM_FRENCH;
		case CONF_LANG_SPANISH:
			return SRAM_SPANISH;
		case CONF_LANG_ITALIAN:
			return SRAM_ITALIAN;
		case CONF_LANG_DUTCH:
			return SRAM_DUTCH;
		default:
			return SRAM_ENGLISH;
	}
}

void GC_SetLanguage()
{
	u8 lang = get_wii_language();

	syssram *sram;
	sram = __SYS_LockSram();
	sram->lang = lang;

	__SYS_UnlockSram(1); // 1 -> write changes
	while(!__SYS_SyncSram());
}

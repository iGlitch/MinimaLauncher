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
#include "patchcode.h"
#include "memory.h"
#include "menu.h"
#include "utils.h"
#include "disc.h"
#include "wdvd.h"
#include "gecko.h"
#include "defines.h"
#include "fst.h"

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
    printf("MinimaLauncher v1.2\n");
    VIDEO_Init();
    WPAD_Init();
    PAD_Init();
    // sleep(1);

    // while (1) {
    //     draw_menu();
    // }

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
        if(Disc_IsWii() == 0)
        {
            FILE *f = NULL;
            size_t fsize = 0;
            /* read in cheats */
            WDVD_ReadDiskId((u8*)Disc_ID);
            const DISC_INTERFACE *sd = &__io_wiisd;
            sd->startup();
            if(sd->isInserted())
                printf("sd inserted!\n");
            fatMountSimple("sd", sd);
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
            /* gct */
            char gamepath[22];
            sprintf(gamepath, "sd:/codes/%.6s.gct", (char*)Disc_ID);
            printf("%s\n", gamepath);
            f = fopen(gamepath, "rb");
            if(f != NULL)
            {
                printf("Opened gct\n");
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
            printf("Using IOS: %i\n", GameIOS);
            WDVD_Close();

            IOS_ReloadIOS(58); //for SDHC support

            /* Re-Init after IOS Reload */
            WDVD_Init();
            WDVD_ReadDiskId((u8*)Disc_ID);
            WDVD_OpenPartition(offset, &GameIOS);

            /* Run Apploader */
            AppEntrypoint = Apploader_Run();
            printf("Entrypoint: %08x\n", AppEntrypoint);
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
                asm volatile(
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


#include <stdio.h>
#include <ogcsys.h>
#include <string.h>
#include <malloc.h>
#include "apploader.h"
#include "wdvd.h"
#include "patchcode.h"
#include "wip.h"
#include "fst.h"
#include "gecko.h"
#include "memory.h"

/* Apploader function pointers */
typedef int   (*app_main)(void **dst, int *size, int *offset);
typedef void  (*app_init)(void (*report)(const char *fmt, ...));
typedef void *(*app_final)();
typedef void  (*app_entry)(void (**init)(void (*report)(const char *fmt, ...)), int (**main)(), void *(**final)());

/* pointers */
static u8 *appldr = (u8*)0x81200000;
static const char *GameID = (const char*)0x80000000;

/* Constants */
#define APPLDR_OFFSET    0x910
#define APPLDR_CODE        0x918

static void PrinceOfPersiaPatch();
static void NewSuperMarioBrosPatch();
static bool Remove_001_Protection(void *Address, int Size);
bool hookpatched = false;

/* Thanks Tinyload */
static struct
{
    char revision[16];
    void *entry;
    s32 size;
    s32 trailersize;
    s32 padding;
} apploader_hdr ATTRIBUTE_ALIGN(32);

u32 Apploader_Run(void)
{
    PrinceOfPersiaPatch();
    NewSuperMarioBrosPatch();

    void *dst = NULL;
    int len = 0;
    int offset = 0;
    u32 appldr_len;
    s32 ret;

    app_entry appldr_entry;
    app_init  appldr_init;
    app_main  appldr_main;
    app_final appldr_final;

    /* Read apploader header */
    ret = WDVD_Read(&apploader_hdr, 0x20, APPLDR_OFFSET);
    if(ret < 0)
        return 0;

    /* Calculate apploader length */
    appldr_len = apploader_hdr.size + apploader_hdr.trailersize;

    /* Read apploader code */
    ret = WDVD_Read(appldr, appldr_len, APPLDR_CODE);
    if(ret < 0)
        return 0;

    /* Flush into memory */
    DCFlushRange(appldr, appldr_len);
    ICInvalidateRange(appldr, appldr_len);

    /* Set apploader entry function */
    appldr_entry = apploader_hdr.entry;

    /* Call apploader entry */
    appldr_entry(&appldr_init, &appldr_main, &appldr_final);

    /* Initialize apploader */
    appldr_init(gprintf);

    while(appldr_main(&dst, &len, &offset))
    {
        /* Read data from DVD */
        WDVD_Read(dst, len, offset);
        do_wip_code((u8 *)dst, len);
        if(hooktype != 0 && hookpatched == false)
            hookpatched = dogamehooks(dst, len, false);
        DCFlushRange(dst, len);
        ICInvalidateRange(dst, len);
        Remove_001_Protection(dst, len);
        Https_Patch(dst, len);
    }
    free_wip();
    if(hooktype != 0 && hookpatched)
        ocarina_do_code();

    /* Set entry point from apploader */
    return (u32)appldr_final();
}

static void PrinceOfPersiaPatch()
{
    if(memcmp("SPX", GameID, 3) != 0 && memcmp("RPW", GameID, 3) != 0)
        return;

    WIP_Code CodeList[5];
    CodeList[0].offset = 0x007AAC6A;
    CodeList[0].srcaddress = 0x7A6B6F6A;
    CodeList[0].dstaddress = 0x6F6A7A6B;
    CodeList[1].offset = 0x007AAC75;
    CodeList[1].srcaddress = 0x7C7A6939;
    CodeList[1].dstaddress = 0x69397C7A;
    CodeList[2].offset = 0x007AAC82;
    CodeList[2].srcaddress = 0x7376686B;
    CodeList[2].dstaddress = 0x686B7376;
    CodeList[3].offset = 0x007AAC92;
    CodeList[3].srcaddress = 0x80717570;
    CodeList[3].dstaddress = 0x75708071;
    CodeList[4].offset = 0x007AAC9D;
    CodeList[4].srcaddress = 0x82806F3F;
    CodeList[4].dstaddress = 0x6F3F8280;
    set_wip_list(CodeList, 5);
}

static void NewSuperMarioBrosPatch()
{
    WIP_Code CodeList[3];
    if(memcmp("SMNE01", GameID, 6) == 0)
    {
        CodeList[0].offset = 0x001AB610;
        CodeList[0].srcaddress = 0x9421FFD0;
        CodeList[0].dstaddress = 0x4E800020;
        CodeList[1].offset = 0x001CED53;
        CodeList[1].srcaddress = 0xDA000000;
        CodeList[1].dstaddress = 0x71000000;
        CodeList[2].offset = 0x001CED6B;
        CodeList[2].srcaddress = 0xDA000000;
        CodeList[2].dstaddress = 0x71000000;
        set_wip_list(CodeList, 3);
    }
    else if(memcmp("SMNP01", GameID, 6) == 0)
    {
        CodeList[0].offset = 0x001AB750;
        CodeList[0].srcaddress = 0x9421FFD0;
        CodeList[0].dstaddress = 0x4E800020;
        CodeList[1].offset = 0x001CEE90;
        CodeList[1].srcaddress = 0x38A000DA;
        CodeList[1].dstaddress = 0x38A00071;
        CodeList[2].offset = 0x001CEEA8;
        CodeList[2].srcaddress = 0x388000DA;
        CodeList[2].dstaddress = 0x38800071;
        set_wip_list(CodeList, 3);
    }
    else if(memcmp("SMNJ01", GameID, 6) == 0)
    {
        CodeList[0].offset = 0x001AB420;
        CodeList[0].srcaddress = 0x9421FFD0;
        CodeList[0].dstaddress = 0x4E800020;
        CodeList[1].offset = 0x001CEB63;
        CodeList[1].srcaddress = 0xDA000000;
        CodeList[1].dstaddress = 0x71000000;
        CodeList[2].offset = 0x001CEB7B;
        CodeList[2].srcaddress = 0xDA000000;
        CodeList[2].dstaddress = 0x71000000;
        set_wip_list(CodeList, 3);
    }
}

static bool Remove_001_Protection(void *Address, int Size)
{
    static const u8 SearchPattern[] = {0x40, 0x82, 0x00, 0x0C, 0x38, 0x60, 0x00, 0x01, 0x48, 0x00, 0x02, 0x44, 0x38, 0x61, 0x00, 0x18};
    static const u8 PatchData[] = {0x40, 0x82, 0x00, 0x04, 0x38, 0x60, 0x00, 0x01, 0x48, 0x00, 0x02, 0x44, 0x38, 0x61, 0x00, 0x18};
    u8 *Addr_end = Address + Size;
    u8 *Addr;

    for(Addr = Address; Addr <= Addr_end - sizeof SearchPattern; Addr += 4)
    {
        if(memcmp(Addr, SearchPattern, sizeof SearchPattern) == 0)
        {
            memcpy(Addr, PatchData, sizeof PatchData);
            return true;
        }
    }
    return false;
}

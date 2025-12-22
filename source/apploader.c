#include <stdio.h>
#include <ogcsys.h>
#include <string.h>
#include <malloc.h>
#include "apploader.h"
#include "wdvd.h"
#include "patchcode.h"
#include "wip.h"
#include "fst.h"
#include "memory.h"

/* Apploader function pointers */
typedef int   (*app_main)(void **dst, int *size, int *offset);
typedef void  (*app_init)(void (*report)(const char *fmt, ...));
typedef void *(*app_final)();
typedef void  (*app_entry)(void (**init)(void (*report)(const char *fmt, ...)), int (**main)(void **, int *, int *), void *(**final)());

/* pointers */
static u8 *appldr = (u8*)0x81200000;
static const char *GameID = (const char*)0x80000000;

/* Constants */
#define APPLDR_OFFSET	0x910
#define APPLDR_CODE		0x918

static bool Remove_001_Protection(void *Address, int Size);
bool hookpatched = false;

bool geckoinit = false;
char gprintfBuffer[256];
void gprintf(const char *format, ...)
{
	va_list va;
	if(geckoinit)
	{
		va_start(va, format);
		int len = vsnprintf(gprintfBuffer, 255, format, va);
		u32 level = IRQ_Disable();
		usb_sendbuffer_safe(1, gprintfBuffer, len);
		IRQ_Restore(level);
		va_end(va);
	}
}

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
		Remove_001_Protection(dst, len);
		Https_Patch(dst, len);
		DCFlushRange(dst, len);
		ICInvalidateRange(dst, len);
	}
	free_wip();
	if(hooktype != 0 && hookpatched)
		ocarina_do_code();

	/* Set entry point from apploader */
	return (u32)appldr_final();
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

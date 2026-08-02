#include <string.h>
#include <ogcsys.h>
#include "apploader.h"
#include "wdvd.h"
#include "patchcode.h"
#include "fst.h"

/* Apploader function pointers */
typedef int   (*app_main)(void **dst, int *size, int *offset);
typedef void  (*app_init)(void (*report)(const char *fmt, ...));
typedef void *(*app_final)(void);
typedef void  (*app_entry)(void (**init)(void (*report)(const char *fmt, ...)), int (**main)(void **, int *, int *), void *(**final)(void));

/* pointers */
static u8 *appldr = (u8*)0x81200000;

/* Constants */
#define APPLDR_OFFSET	0x910
#define APPLDR_CODE		0x918

static void noreport(const char *fmt, ...)
{
	(void)fmt;
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

static void Remove_001_Protection(void *addr, u32 len)
{
	static const u8 SearchPattern[16] = {0x40, 0x82, 0x00, 0x0C, 0x38, 0x60, 0x00, 0x01,
	                                     0x48, 0x00, 0x02, 0x44, 0x38, 0x61, 0x00, 0x18};
	u8 *Addr = (u8*)addr;

	for(u32 i = 0; i + sizeof(SearchPattern) <= len; i += 4)
	{
		if(memcmp(Addr + i, SearchPattern, sizeof(SearchPattern)) == 0)
		{
			Addr[i + 3] = 0x04;
			return;
		}
	}
}

u32 Apploader_Run(void)
{
	void *dst = NULL;
	int len = 0;
	int offset = 0;
	bool hookpatched = false;

	app_entry appldr_entry;
	app_init  appldr_init;
	app_main  appldr_main;
	app_final appldr_final;

	/* Read apploader header */
	if(WDVD_Read(&apploader_hdr, 0x20, APPLDR_OFFSET) < 0)
		return 0;

	/* Calculate apploader length */
	u32 appldr_len = apploader_hdr.size + apploader_hdr.trailersize;

	/* Read apploader code */
	if(WDVD_Read(appldr, appldr_len, APPLDR_CODE) < 0)
		return 0;

	/* Flush into memory */
	DCFlushRange(appldr, appldr_len);
	ICInvalidateRange(appldr, appldr_len);

	/* Set apploader entry function */
	appldr_entry = apploader_hdr.entry;

	/* Call apploader entry */
	appldr_entry(&appldr_init, &appldr_main, &appldr_final);

	/* Initialize apploader */
	appldr_init(noreport);

	while(appldr_main(&dst, &len, &offset))
	{
		/* Read data from DVD */
		WDVD_Read(dst, len, offset);
		if(!hookpatched)
			hookpatched = dogamehooks(dst, len);
		Remove_001_Protection(dst, len);
		Https_Patch(dst, len);
		DCFlushRange(dst, len);
		ICInvalidateRange(dst, len);
	}

	if(hookpatched)
		ocarina_do_code();

	/* Set entry point from apploader */
	return (u32)appldr_final();
}

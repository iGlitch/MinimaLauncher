#include <string.h>
#include <gccore.h>
#include "patchcode.h"

extern void patchhook(u32 address, u32 len);
extern void multidolhook(u32 address);

#if HOOKTYPE == 1
const u32 hookdata[4] = {0x7CE33B78, 0x38870034, 0x38A70038, 0x38C7004C};
#elif HOOKTYPE == 2
const u32 hookdata[4] = {0x9A3F005E, 0x38AE0080, 0x389FFFFC, 0x7E0903A6};
#elif HOOKTYPE == 3
const u32 hookdata[4] = {0x3AB50001, 0x3A73000C, 0x2C150004, 0x3B18000C};
#elif HOOKTYPE == 4
const u32 hookdata[4] = {0x3CA0CC01, 0x38000061, 0x3C804500, 0x98058000};
#elif HOOKTYPE == 5
const u32 hookdata[4] = {0x90010014, 0x800305FC, 0x2C000000, 0x41820008};
#elif HOOKTYPE == 6
const u32 hookdata[4] = {0x90A402E0, 0x806502E4, 0x908502E4, 0x2C030000};
#elif HOOKTYPE == 7
const u32 hookdata[4] = {0x3800000E, 0x7FE3FB78, 0xB0050000, 0x38800080};
#else
#error "unsupported HOOKTYPE"
#endif

static const u32 multidolhooks[4] = {0x7C0004AC, 0x4C00012C, 0x7FE903A6, 0x4E800420};

bool dogamehooks(void *addr, u32 len)
{
	u8 *p = (u8*)addr;
	u8 *end = p + len;
	bool hookpatched = false;

	for(; p < end; p += 4)
	{
		if(memcmp(p, hookdata, sizeof(hookdata)) == 0)
		{
			patchhook((u32)p, len);
			hookpatched = true;
		}
		else if(memcmp(p, multidolhooks, sizeof(multidolhooks)) == 0)
		{
			multidolhook((u32)p + sizeof(multidolhooks) - 4);
			hookpatched = true;
		}
	}
	return hookpatched;
}

void Https_Patch(void *addr, u32 len)
{
	char *cur = (char *)addr;
	const char *end = (char *)addr + len;

	do
	{
		if(memcmp(cur, "https://", 8) == 0 && cur[8] != 0)
		{
			int slen = strlen(cur);
			memmove(cur + 4, cur + 5, slen - 5);
			cur[slen - 1] = 0;
			DCFlushRange((void *)((u32)cur & ~0x1F), ((u32)slen + 0x3F) & ~0x1F);
			cur += slen;
		}
	} while(++cur < end);
}

#ifndef __MEMORY_H_
#define __MEMORY_H_

#define	 Disc_ID		((vu32*)0x80000000)
#define	 Sys_Magic		((vu32*)0x80000020)
#define	 Sys_Version	((vu32*)0x80000024)
#define	 Arena_L		((vu32*)0x80000030)
#define	 Assembler		((vu32*)0x80000060)
#define	 Video_Mode		((vu32*)0x800000CC)
#define  OS_Thread		((vu32*)0x800000E4)
#define	 Dev_Debugger	((vu32*)0x800000EC)
#define	 Simulated_Mem	((vu32*)0x800000F0)
#define	 BI2			((vu32*)0x800000F4)
#define	 Bus_Speed		((vu32*)0x800000F8)
#define	 CPU_Speed		((vu32*)0x800000FC)
#define  Current_IOS	((vu32*)0x80003140)
#define	 Online_Check	((vu32*)0x80003180)
#define	 GameID_Address	((vu32*)0x80003184)
#define  Apploader_IOS	((vu32*)0x80003188)

#endif

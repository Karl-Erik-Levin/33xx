/* cpuDrv.h - Dataton 33xx
**
**	Deals with CPU (AT91SAM7S) specific stuff
** 		
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/


#ifndef _cpuDrv_
#define _cpuDrv_

//--------------------------------PROTOTYPS---------------------------------------
void cpuForceWatchdogReset(void);
void cpuInitWatchDog(void);
void cpuKickWatchDog(void);

void cpuSleep(Boolean fullPowerSave);


#endif	// _cpuDrv_


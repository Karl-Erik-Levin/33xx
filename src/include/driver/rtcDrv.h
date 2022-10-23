/* rtcDrv.h - Dataton 33xx
**
** Real-time clock driver.
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

#ifndef _rtcDrv_
#define _rtcDrv_

//--------------------------------PROTOTYPS---------------------------------------
void rtcInit(void);
void rtcClose(void);

void rtcSleep(LongWord wakeInNumMs);
void rtcWake(void);

void rtcSetRtValue(LongWord rtValue);
LongWord rtcGetRtValue(void);
LongWord rtcMSToTicks(LongWord ms);


#endif	// _rtcDrv_

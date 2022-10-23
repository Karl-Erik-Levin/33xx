/* tmrDrv.h - Dataton 33xx 
**
** This module implements two independent timers. One millisecond and one
** microsecond. Booth are generated from a single 16-bit hardware Timer 1.
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/

#ifndef _tmrDrv_
#define _tmrDrv_

//-------------------Macro to compare two time values----------------------------
#define TM_GT(t1,t2)	((long)((t1)-(t2))>0)
#define TM_GE(t1,t2)	((long)((t1)-(t2))>=0)
#define TM_LT(t1,t2)	((long)((t1)-(t2))<0)
#define TM_LE(t1,t2)	((long)((t1)-(t2))<=0)

//--------------------------------PROTOTYPS---------------------------------------
void tmrInit();
void tmrClose();
void tmrSleep();
void tmrWake();

void tmrWaitUS(LongWord numUs);
void tmrWaitMS(LongWord numMs);

LongWord tmrGetUS();
LongWord tmrGetMS();


#endif	// _tmrDrv_


/* radDrv.h
**
** Interface Pickup2 to FM radio chip Silicon Labs Si4703-C19-GM
**
** (C)Copyright 2006 Dataton Utvecklings AB, All Rights Reserved.
**
** Created	22-09-10	Kalle
** 
*/

#ifndef _radDrv_
#define _radDrv_

//-------------------------------DEFINITIONS--------------------------------------
void rddInit(void);
void rddWake();
void rddSleep();

void rddSetFreq(Word frequency);		// Freq in KHz 760..1080
void rddSetVol(Byte vol);				// 0..15
Byte rddSeek(Byte seekDirection);		// UP = true
Word rddGetFreq();

Byte rddGetSignalStrength();
void rddMono(Boolean activate);

#endif	// _radDrv_

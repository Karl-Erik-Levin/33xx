/* ledMng.h - Dataton 3397 Transponder 
**
**	LED Manager
** 		
**	Display pattern
**
**
** Created 10-03-18	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
**
**/

#ifndef _ledMng_
#define _ledMng_

//---------------------------------TYPEDEFS---------------------------------------
typedef enum {
	kLedMaint,
	kLedInit,
	kLedEnd,
	kLedCenter,
	kLedRing
} ledStt;

//--------------------------------PROTOTYPS---------------------------------------
void ledDisplay(ledStt newState);


#endif	//_ledMng_




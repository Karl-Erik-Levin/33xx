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
	kLedInit,
	kLedEnd,
	kLedCenter,
	kLedRing,
	kLedCenterBlink,
	kLedBattChg,
	kLedUsbData
} ledStt;

//--------------------------------PROTOTYPS---------------------------------------
void ledMaint();						// Maintenace call at 200 Hz
void ledDisplay(ledStt newState);		// Set new display mode


#endif	//_ledMng_




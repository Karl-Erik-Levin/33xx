/* ledMng.c - Dataton 3397 Transponder 
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

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"

#include "driver\ledDrv.h"

#include "platform\ledMng.h"

//----------------------------GLOBAL VARIABLES------------------------------------
static LongWord gMainLoopCnt;
static ledStt ledState;

//--------------------------------FUNCTIONS---------------------------------------
void
ledMaint()
{
	static LongWord internalLoop;
	LongWord idx;
	
	if (gMainLoopCnt > 0) {
		gMainLoopCnt++;
		
		switch (ledState) {
		
		case kLedCenterBlink:
			if (!(gMainLoopCnt%100)) {
				if (!(internalLoop%2)) {
					ledSetLedLevel(LEDI_Center, LEDD_LEVEL_MAX);
				} else {
					ledSetLedLevel(LEDI_Center, LEDD_LEVEL_MIN);
				}
				internalLoop++;
			}
			break;
		
		case kLedCenter:
			if (gMainLoopCnt >= 4) {
				ledSetLedLevel(LEDI_Center, LEDD_LEVEL_MIN);
				gMainLoopCnt = 0;								// Stop maint
			}
			break;
		
		case kLedRing:
			if (gMainLoopCnt >= 21) {
				for (idx=0; idx < LEDD_NUM_RING_LEDS; idx++)
					ledSetLedLevel(idx, LEDD_LEVEL_MIN);
				gMainLoopCnt = 0;								// Stop maint
			} else if (gMainLoopCnt >= 14) {
				for (idx=0; idx < LEDD_NUM_RING_LEDS; idx++)
					ledSetLedLevel(idx, LEDD_LEVEL_MAX);
			} else if (gMainLoopCnt >= 7) {
				for (idx=0; idx < LEDD_NUM_RING_LEDS; idx++)
					ledSetLedLevel(idx, LEDD_LEVEL_MIN);
			}
			break;
			
		case kLedBattChg:
			if (!(gMainLoopCnt%50)) {							// Charge LED indication
				ledSetLedLevel(LEDI_Front, internalLoop%6);		// 0..5
				internalLoop++;
			}
			break;
			
		case kLedUsbData:
			if (gMainLoopCnt > 200)
				gMainLoopCnt = 0;								// Stop after 1 second
			else if (!(gMainLoopCnt%10)) {						// Data LED indication
				if (internalLoop%2)
					ledSetLedLevel(LEDI_Front, LEDD_LEVEL_MAX);
				else
					ledSetLedLevel(LEDI_Front, LEDD_LEVEL_MIN);
				
				internalLoop++;
			}
			break;
		}
	}
}

void
ledDisplay(ledStt newState)
{
	LongWord idx;
	
	switch (newState) {
	
	case kLedInit:
	case kLedEnd:
		gMainLoopCnt = 0;									// Stop maint
		for (idx=0; idx < LEDD_NUM_RING_LEDS+1; idx++)		// All LED off
			ledSetLedLevel(idx, LEDD_LEVEL_MIN);
		break;
		
	case kLedCenterBlink:
		gMainLoopCnt = 1;									// Enable maint
		ledState = kLedCenterBlink;
		break;
		
	case kLedCenter:
		ledSetLedLevel(LEDI_Center, LEDD_LEVEL_MAX);
		for (idx=0; idx < LEDD_NUM_RING_LEDS; idx++)
			ledSetLedLevel(idx, LEDD_LEVEL_MIN);
			
		gMainLoopCnt = 1;									// Enable maint
		ledState = kLedCenter;
		break;
		
	case kLedRing:
		ledSetLedLevel(LEDI_Center, LEDD_LEVEL_MIN);
		for (idx=0; idx < LEDD_NUM_RING_LEDS; idx++)
			ledSetLedLevel(idx, LEDD_LEVEL_MAX);
			
		gMainLoopCnt = 1;									// Enable maint
		ledState = kLedRing;
		break;
		
	case kLedBattChg:
		ledSetLedLevel(LEDI_Front, LEDD_LEVEL_MIN);
		gMainLoopCnt = 1;									// Enable maint
		ledState = kLedBattChg;
		break;
		
	case kLedUsbData:
		ledSetLedLevel(LEDI_Front, LEDD_LEVEL_MIN);
		gMainLoopCnt = 1;									// Enable maint
		ledState = kLedUsbData;
		break;
		
	}
}




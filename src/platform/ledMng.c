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

//--------------------------------FUNCTIONS---------------------------------------
void
ledDisplay(ledStt newState)
{
	static ledStt ledState;
	static LongWord cnt1, cnt2;

	switch (newState) {
	
	case kLedMaint:
		cnt1++;
		switch (ledState) {
		
		case kLedMaint:
			if (!(cnt2%100)) {
				ledSetLedLevel(LEDI_Right, LEDD_LEVEL_MAX);
			} else {
				ledSetLedLevel(LEDI_Right, LEDD_LEVEL_MIN);
			}
			cnt2++;
			break;
		
		case kLedCenter:
			if (cnt1 >= 4) {
				ledSetLedLevel(LEDI_Center, LEDD_LEVEL_MIN);
				ledState = kLedMaint;
			}
			break;
		
		case kLedRing:
			if (cnt1 >= 21) {
				for (cnt2=0; cnt2 < LEDD_NUM_RING_LEDS; cnt2++)
					ledSetLedLevel(cnt2, LEDD_LEVEL_MIN);
				ledState = kLedMaint;
				cnt2 = 1;
			} else if (cnt1 >= 14) {
				for (cnt2=0; cnt2 < LEDD_NUM_RING_LEDS; cnt2++)
					ledSetLedLevel(cnt2, LEDD_LEVEL_MAX);
			} else if (cnt1 >= 7) {
				for (cnt2=0; cnt2 < LEDD_NUM_RING_LEDS; cnt2++)
					ledSetLedLevel(cnt2, LEDD_LEVEL_MIN);
			}
			break;
		}
		break;
		
	case kLedInit:
		cnt1 = cnt2 = 0;
		ledState = kLedMaint;
		break;
		
	case kLedEnd:
		for (cnt1=0; cnt1 < LEDD_NUM_RING_LEDS+1; cnt1++)
			ledSetLedLevel(cnt1, LEDD_LEVEL_MIN);
		break;
		
	case kLedCenter:
		ledSetLedLevel(LEDI_Center, LEDD_LEVEL_MAX);
		for (cnt1=0; cnt1 < LEDD_NUM_RING_LEDS; cnt1++)
			ledSetLedLevel(cnt1, LEDD_LEVEL_MIN);
		cnt1 = 0;
		cnt2 = 1;
		ledState = kLedCenter;
		break;
		
	case kLedRing:
		ledSetLedLevel(LEDI_Center, LEDD_LEVEL_MIN);
		for (cnt2=0; cnt2 < LEDD_NUM_RING_LEDS; cnt2++)
			ledSetLedLevel(cnt2, LEDD_LEVEL_MAX);
		cnt1 = 0;
		ledState = kLedRing;
		break;
	}
}




/* radMng.h
**
** FM radio manager
**
**
** (C)Copyright Dataton Utvecklings AB 2007, All Rights Reserved.
** 
** Created	22-09-06	Kalle
**
*/

#ifndef _radMng_
#define _radMng_

//---------------------------------INCLUDES---------------------------------------
#include "driver\ledDrv.h"					// To get LEDM_NUM_RING_LEDS

//----------------------------------DEFINES---------------------------------------
#define rdmNUM_SLOTS	(LEDM_NUM_RING_LEDS)

//-------------------------------DEFINITIONS--------------------------------------
void rdmInit();
void rdmClose();

void rdmEnable();
void rdmDisable();

void rdmSleep();
void rdmWake();

void rdmMaint();
void rdmSetSquelchLevel(Word level);
void rdmMute(Boolean activate);
void rdmMono(Boolean activate);

Boolean rdmIsRadioActive();
Boolean rdmIsSquelched();
Boolean rdmIsMuted();

void rdmSeek(Boolean up, Boolean save);
void rdmSetFreq(LongWord freqIn100KHz, Boolean save);
void rdmSaveSlotInfo(Boolean persistent);

Word rdmNextSlot();
Word rdmPrevSlot();
Word rdmSlotIndex();
Word rdmGetSlotFreq();
Word rdmGetTunedFreq();
Word rdmGetNumSlots();

#endif	// _radMng_

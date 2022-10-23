/* audMng.c - Dataton 3397 Transponder 
**
** Audio manager
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
** 
*/
#ifndef _audMng_
#define _audMng_

//----------------------------------DEFINES---------------------------------------
#define PB_VOLUME_MAX			100

//-------------------------------DEFINITIONS--------------------------------------
void PB_Init();
void PB_Close();

// Plaback control functions.
void PB_Play(Word sampleRate);
void PB_Stop();

void PB_SetVolume(Word newVolume);

#endif	// _audMng_

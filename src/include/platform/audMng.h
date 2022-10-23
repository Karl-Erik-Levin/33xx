/* audMng.c - Dataton 3356 Pickup
**
** Audio manager
**
**
** Created 10-10-30	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
** 
*/
#ifndef _audMng_
#define _audMng_

//----------------------------------DEFINES---------------------------------------
#define PB_VOLUME_MAX			100

//-------------------------------DEFINITIONS--------------------------------------
void pbInit(void *bigBuffer);
void pbClose();

// Plaback control functions.
void pbInitPlayMP3(LongWord startCluster, LongWord fileSize);
void pbInitPlaySin(Word sampleRate);
void pbInitPlayWav(char *filename);		// Not implemented yet

void pbPlay();
void pbPause();
void pbStop();

void pbSetVolume(Word newVolume);

#endif	// _audMng_

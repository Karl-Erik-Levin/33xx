/* audDrv.h - Dataton 3356 Pickup
**
** Audio out Driver.
**
** Handle Maxim 9850 chip via AT91SAM7S I2S and I2C interface
**
** Function: Manages communication with the audio hardware (Maxim DAC,
** speaker amplifier, etc). Provides services to control volume, set the
** playback rate, control speaker, powersaving and to transmit data the
** device.
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

#ifndef _audDrv_
#define _audDrv_

//---------------------------------DEFINES----------------------------------------
#define AD_PLAYBACK_RATE_MIN	8000	
#define AD_PLAYBACK_RATE_MAX	48000
#define AD_VOLUME_MIN 			0
#define AD_VOLUME_MAX			100

//--------------------------------PROTOTYPS---------------------------------------
void audInit();
void audClose();
void audSleep();
void audWake();
void audSpeakerAmp(Boolean enable);

void audSetPlaybackRate(Word samplesPerSec);
void audSetVolume(Word volume);
void audSetMute(Boolean mute);

void audFlush();
Boolean audPlayBuffer(short *samples, LongWord numSamples);
LongWord audGetBufferCount();

void audEnableRadio(void);
void audDisableRadio(void);

typedef enum {
	EADU_PlaybackManager = 0,
	EADU_RadioManager = 1
} EADUser;

void audRegUser(EADUser user);
void audUnregUser(EADUser user);



#endif  // _audDrv_



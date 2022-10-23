/* adcDrv.h - Dataton 33xx
**
**	uProcessor A/D converter Driver
** 		
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/

#ifndef _adcDrv_
#define _adcDrv_

//---------------------------------DEFINES----------------------------------------
typedef enum
{
	EADC_Headphones = 0,		
	EADC_Volume,
	EADC_BatteryLevel,
//	EADC_BatteryCurrent,
	_EADC_NumChannels
} EADCChannel;

#define POTD_LEVEL_MIN		0
#define POTD_LEVEL_MAX		1024

//--------------------------------PROTOTYPS---------------------------------------
void adcInit();
Word adcGetLevel(EADCChannel ch);
void adcSleep();
void adcWake();

#endif	// _adcDrv_


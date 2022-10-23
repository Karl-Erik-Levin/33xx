/* ledDrv.h - Dataton 33xx
**
** LED driver. Handle both Pickup and Transponder hardware
**
** Pickup 3356. Using the Maxim LEDDriver MAX6964, connected to the I2C bus, 
** to control the 13 leds in the button and the front led.
**
** Transponder 3397. Control LED on and off via PIO.
**
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/


#ifndef _ledDrv_
#define _ledDrv_

//---------------------------------DEFINES----------------------------------------
typedef Byte LEDDIndex;
static const LEDDIndex LEDI_Top   = 0;				// Ring top led.
static const LEDDIndex LEDI_Right = 3;				// Ring right led.
static const LEDDIndex LEDI_Bottom= 6;				// Ring bottom led.
static const LEDDIndex LEDI_Lefe  = 9;				// Ring left led.
static const LEDDIndex LEDI_Center= 12;				// Ring center led.
static const LEDDIndex LEDI_Front = 13;				// Front led. Only in Pickup hardware

#define LEDD_LEVEL_MIN		0						// Minimum level of a led.
#define LEDD_LEVEL_MAX		15						// Maximum level of a led.
#define LEDD_NUM_RING_LEDS	12						// Number of leds in the ring.

//--------------------------------PROTOTYPS---------------------------------------
void ledInit(void);
void ledSetGlobLevel(Byte level);
void ledSetLedLevel(LEDDIndex ledIX, Byte ledLevel);
void ledWake();
void ledSleep();

#endif	// _ledDrv_





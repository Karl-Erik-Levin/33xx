/* ledDrv.c - Dataton 33xx
**
** LED driver. Handle both Pickup and Transponder hardware
**
** Pickup 3356. Using the Maxim LEDDriver MAX6964, connected to the I2C bus, 
** to control the 13 leds in the button and the front led.
**
** Transponder 3397. Control LED on and off via PIO.
**
**
** From the pickup users point of wiev:
**
**  Posision  LED  MAX6964
**  Left      D1   Out0
**	Down      D4   Out3
**  Right     D7   Out6
**  Top       D10  Out9
**  Center    D17  Out12
**  Front     D18  Out13
**
**
** Dependencies:
** I2C driver 
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"

#include "driver\hwBoard.h"
#include "driver\ledDrv.h"

#if (HARDWARE==3397)

#define AllLED (LED_01 | LED_02 | LED_03 | LED_04 |			\
				LED_05 | LED_06 | LED_07 | LED_08 |			\
				LED_09 | LED_10 | LED_11 | LED_12 | LED_13)
				
//------------------------------GLOBAL VARIABLES----------------------------------
static unsigned int ledTab[13] = {LED_01, LED_02, LED_03, LED_04,
								  LED_05, LED_06, LED_07, LED_08,
								  LED_09, LED_10, LED_11, LED_12, LED_13};
								  
//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	ledInit
 * Summary:		Initiate LED driver
 *******************************************************************************/
void
ledInit(void)
{
	AT91C_BASE_PIOA->PIO_PER = AllLED;				// LED control pin in PIO mode
	AT91C_BASE_PIOA->PIO_OER = AllLED;				// LED control pin configure as output
	AT91C_BASE_PIOA->PIO_MDER =AllLED;				// LED control pin configure in open drain mode
	AT91C_BASE_PIOA->PIO_PPUDR=AllLED;				// LED control pin disable internal pullup

	AT91C_BASE_PIOA->PIO_SODR=AllLED;				// LED control pin Set output high. LED Off
}

/*******************************************************************************
 * Function:	ledSetGlobLevel
 * Summary:		
 *******************************************************************************/
void
ledSetGlobLevel(Byte level)
{

}

/*******************************************************************************
 * Function:	ledSetLedLevel
 * Summary:		
 *******************************************************************************/
void
ledSetLedLevel(LEDDIndex ledIX, Byte ledLevel)
{
	if (ledLevel)
		AT91C_BASE_PIOA->PIO_CODR = ledTab[ledIX];
	else
		AT91C_BASE_PIOA->PIO_SODR = ledTab[ledIX];
}

/*******************************************************************************
 * Function:	ledWake
 * Summary:		
 *******************************************************************************/
void
ledWake()
{

}

/*******************************************************************************
 * Function:	ledSleep
 * Summary:		
 *******************************************************************************/
void
ledSleep()
{

}


#elif (HARDWARE==3356)
//---------------------------------INCLUDES---------------------------------------
#include "driver\i2cDrv.h"
#include "driver\tmrDrv.h"

//------------------------------GLOBAL VARIABLES----------------------------------
static Byte gLevels[9]  = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static Byte gBlkReg0[2] = {0xff, 0xff};

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	resetMAX6964
 * Summary:		Reset the LED circuit
 *******************************************************************************/
static void
resetMAX6964()
{
	/* Make a 10 ms long reset pulse to power down led circuit MAX6964 */
	AT91F_PIO_ClearOutput(AT91C_BASE_PIOA, TWI_LED_RESET_PIN);	// Assert reset signal
	tmrWaitMS(10);
	AT91F_PIO_SetOutput(AT91C_BASE_PIOA, TWI_LED_RESET_PIN);	// Release reset signal
	tmrWaitMS(10);
}

/*******************************************************************************
 * Function:	ledInit
 * Summary:		Initiate LED driver
 *******************************************************************************/
void
ledInit(void)
{
	Byte txBuf[1];
	
	AT91F_PIO_CfgOutput(AT91C_BASE_PIOA, TWI_LED_RESET_PIN);	// Config reset signal as output
	AT91C_BASE_PIOA->PIO_PPUDR = TWI_LED_RESET_PIN;				// Pullup disable
	
	resetMAX6964();									// Send reset pulse to LED driver circuit
	
	i2cWrite(EI2CD_6964, 0x0E, &gLevels[8], 1);		// Restore MASTER INTENSITY
	tmrWaitMS(1);									// 1 ms delay

	txBuf[0] = 0x30;
	i2cWrite(EI2CD_6964, 0x0F, txBuf, 1);			// Disable BLINK and GLOBAL INTENSITY
	tmrWaitMS(1);									// 1 ms delay
	
	i2cWrite(EI2CD_6964, 0x02, gBlkReg0, 2);		// Set BLINK reg0, 1=Off 
	tmrWaitMS(1);									// 1 ms delay

	i2cWrite(EI2CD_6964, 0x10, gLevels, 8);			// Set all LED levels
	tmrWaitMS(1);									// 1 ms delay
}

/*******************************************************************************
 * Function:	ledSetGlobLevel
 * Summary:		Set global light level 1..15. 0 is PWM disable ie only off and on
 *******************************************************************************/
void
ledSetGlobLevel(Byte level)
{
	gLevels[8] = (Byte) ((level << 4) | gLevels[8] & 0x0F);
	
	i2cWrite(EI2CD_6964, 0x0E, &gLevels[8], 1);
}

/*******************************************************************************
 * Function:	ledSetLedLevel
 * Summary:		ledLevel 0 is off and 15 is max light level
 *				ledIX == 0  LED at 12 
 *						 1  LED at 1
 *						 2  LED at 2     and so on
 *			 			 12 center LED
 *			 			 13 front LED
 *******************************************************************************/
void
ledSetLedLevel(LEDDIndex inLedIX, Byte ledLevel)
{
	int lvlRegIdx;
	int blkRegIdx;
	Byte oldLevel;
	LEDDIndex ledIX;

	inLedIX &= 0x0F;		// Valid value 0..15
	if (inLedIX<10)
		ledIX = 9 - inLedIX;
	else
		ledIX = 21- inLedIX;
	
	if (inLedIX>11)
		ledIX = inLedIX;
	
	lvlRegIdx = ledIX / 2;
	blkRegIdx = ledIX / 8;
	
	ledLevel &= 0xf;		// Valid value 0..15
	if (ledLevel == 0) {
		ledLevel = 0xf;
	} else {
		ledLevel--;
	}
	
	if (ledIX & 1) {
		oldLevel = (gLevels[lvlRegIdx] >> 4) & 0xf;
	} else {
		oldLevel = gLevels[lvlRegIdx] & 0xf;
	}
	
	
	if (ledLevel != oldLevel) {
	
		if ((ledLevel==0xf) || (oldLevel==0xf)) {
			if (ledLevel == 0xf) {
				gBlkReg0[blkRegIdx] |= (1 << (ledIX % 8));
			} else {
				gBlkReg0[blkRegIdx] &=~(1 << (ledIX % 8));
			}
			i2cWrite(EI2CD_6964, 0x02 + blkRegIdx, &gBlkReg0[blkRegIdx], 1);
		}
		
		if (ledIX & 1)
		{
			gLevels[lvlRegIdx] &= 0x0f;
			gLevels[lvlRegIdx] |= (ledLevel << 4);
		}
		else
		{
			gLevels[lvlRegIdx] &= 0xf0;
			gLevels[lvlRegIdx] |= ledLevel;
		}
		i2cWrite(EI2CD_6964, 0x10 + lvlRegIdx, &gLevels[lvlRegIdx], 1);
	}
}

/*******************************************************************************
 * Function:	ledWake
 * Summary:		
 *******************************************************************************/
void ledWake()
{
	ledInit();				// Initialize chip and set old levels
	
	i2cWrite(EI2CD_6964, 0x02, gBlkReg0, 2);		// Set BLINK reg0, 1=Off 
	tmrWaitMS(1);										// 1 ms delay

	i2cWrite(EI2CD_6964, 0x10, gLevels, 8);			// Set all LED levels
	tmrWaitMS(1);										// 1 ms delay
}

/*******************************************************************************
 * Function:	ledSleep
 * Summary:		
 *******************************************************************************/
void ledSleep()
{
	resetMAX6964();			// Will set LED driver circuit in low power mode
}

#endif	// (HARDWARE==3356)

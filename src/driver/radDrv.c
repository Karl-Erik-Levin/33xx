/* radDrv.c
**
** Interface Pickup2 to FM radio chip Silicon Labs Si4703-C19-GM
**
** The chip is comunicating over I2C, it need a reference clock 32768 Hz
** and it also have RDS
**
** (C)Copyright 2022 Dataton Utvecklings AB, All Rights Reserved.
**
** Created	22-09-10	Kalle
** 
*/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"

#include "driver\hwBoard.h"
#include "driver\cpuDrv.h"
#include "driver\i2cDrv.h"
#include "driver\radDrv.h"

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"

//----------------------------GLOBAL VARIABLES------------------------------------
static Byte RadioReg[8];
static Word gSleepFreq;

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	enable32KHz
 * Summary:		Feed 32768 Hz clock to radio
 *				Timer 2 is used to generate 32 KHz
 *******************************************************************************/
static void
enable32KHz(void)
{
	AT91F_PMC_EnablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_TC2);
	AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA, 0, SI4703_32KHZ_CLK_PIN);
	
	AT91C_BASE_TC2->TC_CMR =
		AT91C_TC_ACPC_TOGGLE |
		AT91C_TC_WAVESEL_UP_AUTO |
		AT91C_TC_WAVE |
		AT91C_TC_CLKS_TIMER_DIV1_CLOCK;
	
	AT91C_BASE_TC2->TC_RC = 732/2;
	
    AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKEN; 
    AT91C_BASE_TC2->TC_CCR = AT91C_TC_SWTRG;  	// Kick start clock
    
    vTaskDelay(100);
}

/*******************************************************************************
 * Function:	resetPulse
 * Summary:		Send a reset pulse to radio chip Si4703, active low
 *******************************************************************************/
static void
resetPulse()
{
	AT91F_PIO_CfgOutput(AT91C_BASE_PIOA, SI4703_INT);

	AT91F_PIO_ClearOutput(AT91C_BASE_PIOA, SI4703_INT);
	vTaskDelay(10);
	
	AT91F_PIO_SetOutput(AT91C_BASE_PIOA, SI4703_INT);
}

/*******************************************************************************
 * Function:	rddInit
 * Summary:		
 *******************************************************************************/
void
rddInit(void)
{
	Byte buf[2];

	resetPulse();
	enable32KHz();

	i2cRead(EI2CD_9850, 0, &buf[0], 1);
	i2cRead(EI2CD_9850, 1, &buf[1], 1);

	RadioReg[0] = 0x40;		// Mute disable
	RadioReg[1] = 0x01;		// Power up chip
	RadioReg[2] = 0x00;						
	RadioReg[3] = 0x00;
	RadioReg[4] = 0x10;		// Enable rddS
	RadioReg[5] = 0x00;	
	RadioReg[6] = 0x1C;		// Seek threshold = 29
	RadioReg[7] = 0x58;		// Band = 1 (76-108 MHz), Volume = 8
  
	i2cWrite(EI2CD_4703, 0x00, RadioReg, 2);   // Power on
  
	rddSetVol(0);
}

/*******************************************************************************
 * Function:	rddSleep
 * Summary:		
 *******************************************************************************/
void
rddSleep()
{
	gSleepFreq = rddGetFreq();
	
	RadioReg[1] = 0x41;                          
	i2cWrite(EI2CD_4703, 0x00, RadioReg, 2);	// Power down radio chip
	
	AT91C_BASE_TC2->TC_CCR = AT91C_TC_CLKDIS; 	// Stop 32 kHz clock.
}

/*******************************************************************************
 * Function:	rddWake
 * Summary:		
 *******************************************************************************/
void
rddWake()
{
	enable32KHz();
	
	RadioReg[1] = 0x01;                       
	i2cWrite(EI2CD_4703, 0x00, RadioReg, 2);   // Power up chip

	rddSetFreq(gSleepFreq);
}

/*******************************************************************************
 * Function:	rddSetVol
 * Summary:		
 *******************************************************************************/
void
rddSetVol(Byte vol)							// 0..15
{
	RadioReg[7] &= 0xF0;
	RadioReg[7] |= (vol & 0x0F);
	
	i2cWrite(EI2CD_4703, 0x00, RadioReg, 8);	// Set volume
	vTaskDelay(10);
}

/*******************************************************************************
 * Function:	rddMono
 * Summary:		
 *******************************************************************************/
void rddMono(Boolean activate)
{

	RadioReg[0] = activate ? RadioReg[0] | 0x20 : RadioReg[0] & ~0x20;
	i2cWrite(EI2CD_4703, 0x00, RadioReg, 1);
}

/*******************************************************************************
 * Function:	rddSetFreq
 * Summary:		
 *******************************************************************************/
void
rddSetFreq(Word freq)
{							// In 100 KHZ step
	Byte rxBuf[2];
	
	freq -= 759;
	
	RadioReg[2] = 0x80 | (Byte)(freq>>8);		// Tune channel freq
	RadioReg[3] = freq;
	
	i2cWrite(EI2CD_4703, 0x00, RadioReg, 4);
	
	do {										// Wait for radio to become ready
		vTaskDelay(5);
		i2cRead(EI2CD_4703, 0x00, rxBuf, 2);
	} while (!(rxBuf[0] & 0x40));
	
	RadioReg[2] = 0x00;							// Reset tune flag
	i2cWrite(EI2CD_4703, 0x00, RadioReg, 4);
}

/*******************************************************************************
 * Function:	rddSeek
 * Summary:		
 *******************************************************************************/
Byte
rddSeek(Byte up)
{
	Byte rxBuf[2];
	
	if (up)
		RadioReg[0] |= 0x02;
	else
		RadioReg[0] &=~0x02;
	
	RadioReg[0] |= 0x01;   // Set seek flag
	i2cWrite(EI2CD_4703, 0x00, RadioReg, 4);
	
	do {                  // Wait for radio to become ready
		vTaskDelay(5);
		cpuKickWatchDog();
		i2cRead(EI2CD_4703, 0x00, rxBuf, 2);
	} while (!(rxBuf[0] & 0x40));
	
	RadioReg[0] &=~0x01;   // Reset seek flag
	i2cWrite(EI2CD_4703, 0x00, RadioReg, 2);
	
	return 0;
}

/*******************************************************************************
 * Function:	rddGetFreq
 * Summary:		
 *******************************************************************************/
Word
rddGetFreq()
{
	Byte freqBuf[4];

	i2cRead(EI2CD_4703, 0x00, freqBuf, 4);

	return (Word)759 + (Word)((freqBuf[2] & 1) << 8) + freqBuf[3];
}

/*******************************************************************************
 * Function:	rddGetSignalStrength
 * Summary:		
 *******************************************************************************/
Byte rddGetSignalStrength()
{
	Byte ssBuf[2];

	i2cRead(EI2CD_4703, 0x00, ssBuf, 2);

	return ssBuf[1];
}


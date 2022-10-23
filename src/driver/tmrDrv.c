/* tmrDrv.c - Dataton 33xx 
**
** This module implements two independent timers. One millisecond and one
** microsecond. Booth are generated from a single 16-bit hardware Timer 1.
** (Timer 0 is used by IrDA Tx and Timer 2 is used to generate 32 kHz to radio)
**
** T1 interrupts are generated with ~1000 Hz, on interrupt the MS timer is updated,
** and any drift are compensated for.
**
** KALLE COMMENT! This is crazy because FreeRTOS also use another interrupt at
** approx 1000 Hz with the PIT
**
** With the 18.432 mHz crystal the master clock is set to MCK = 47 923 200 Hz
** 
** Note: When T1 reaches the value written in time register C(RC) it automatically
** resets and start counting from 0 again.
**
** History:
** 20050331 	Fred J		Fixed bug, TC is reset when RC-value is reached.		
** 20060609		Fred J		Added ms and uS timer drift compensation.
** 20080319		Kalle		Uppdaterat kommentarer
** 20081107		Kalle		Remove alarm function because it's not used
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/


#include "MFLib\Dataton_Types.h"

#include "driver\hwBoard.h"
#include "driver\tmrDrv.h"

#include "platform\FreeRTOS\FreeRTOS.h"


static LongWord 	usIntTicks = 0;
static Word 		usDriftCnt = 0;
static long			msIntTick = 0;
static Word 		msDriftCnt = 0;

#define kTimerInterval 		(47924/2)	// Timer 1 is clocked by MCK = 47 923 200 Hz
#define kUsPerInterval 		1000
#define kTimerTicksPerUs 	24
#define kUSTimerIRQPrio 	4

extern void usTimerISREntry(void);
extern void usTimerISR(void);


#pragma ghs nothumb
void usTimerISR(void)
{	
	AT91_REG dummy = AT91C_BASE_TC1->TC_SR; 
	
	usIntTicks += kUsPerInterval;

	/***********************************************
	  Manage timer drift.
	***********************************************/
	if ((++usDriftCnt) >= 5)
	{
		// Drift 0.8 uS per interrupt. Compensate for that by skipping
		// 4 (0.8 * 5) uS every 5:th interrupt.
		usIntTicks += 4;
		usDriftCnt = 0;
	}

	if ((++msDriftCnt) < (1250 * 9))
	{
		msIntTick++;
	}
	else
	{
		msIntTick += 2;
		msDriftCnt = 0;
	}
	
	AT91C_BASE_TC1->TC_RC = kTimerInterval;		// Needed?
}

void
tmrInit()
{
	AT91_REG dummy;
	
	AT91F_AIC_DisableIt(AT91C_BASE_AIC, AT91C_ID_TC1);
	AT91F_PMC_EnablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_TC1);
	
	AT91C_BASE_TC1->TC_CCR = AT91C_TC_CLKDIS;			// Timer1: Disable timer
	AT91C_BASE_TC1->TC_IDR = 0xffffffff;				// Timer1: Disable all interrupts
	AT91C_BASE_TCB->TCB_BMR = 1 << 4 | 1 << 2 | 1 << 0;	// All timers clocked through internal oscilator.
	
	// Timer1: Capture Mode + RC Compare Trigger Enable + TIMER_DIV1_CLOCK ==> Clock source is MCK/2
	AT91C_BASE_TC1->TC_CMR =  AT91C_TC_CPCTRG | AT91C_TC_CLKS_TIMER_DIV1_CLOCK;
	dummy = AT91C_BASE_TC1->TC_SR;						// Timer1: Read Status Register ????

	AT91F_AIC_ConfigureIt(							// Setup interrupt from Timer 1
				AT91C_BASE_AIC, 
				AT91C_ID_TC1, 
				kUSTimerIRQPrio,
				AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE,
				usTimerISREntry);
	
	AT91C_BASE_TC1->TC_RC = kTimerInterval;			// Timer 1: Set compare register to generate interrupt every ms
	AT91C_BASE_TC1->TC_IER = AT91C_TC_CPCS;			// Timer 1: Enable interrupt RC Compare

	AT91C_BASE_TC1->TC_CCR = AT91C_TC_CLKEN;		// Timer1: Enable timer
    AT91C_BASE_TC1->TC_CCR = AT91C_TC_SWTRG ;		// Timer1: SW trigg to reset and start timer 
	AT91F_AIC_ClearIt(AT91C_BASE_AIC, AT91C_ID_TC1);
	AT91F_AIC_EnableIt(AT91C_BASE_AIC, AT91C_ID_TC1);
}

void
tmrClose()
{
	AT91F_AIC_DisableIt(AT91C_BASE_AIC, AT91C_ID_TC1);
	AT91C_BASE_TC1->TC_IDR = AT91C_TC_CPCS;
	AT91F_AIC_ClearIt(AT91C_BASE_AIC, AT91C_ID_TC1);
}

/* Put timer driver to sleep to conserve power.
**
** This is done by disabling interrupts and to detatch the PMC.
** Detatching the PMC will put the entire timer I/O
** module to a stand still.
*/
void
tmrSleep()
{
	AT91F_AIC_DisableIt(AT91C_BASE_AIC, AT91C_ID_TC1);
	AT91F_PMC_DisablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_TC1);
}

/* Wake timers after sleep! */
void
tmrWake()
{
	AT91F_PMC_EnablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_TC1);
	AT91F_AIC_EnableIt(AT91C_BASE_AIC, AT91C_ID_TC1);
}

/* Get the current uS timer value
**
** Note: Do not call from interupts!
**
*/
LongWord
tmrGetUS()
{
	long now;
	long usIntTicksCpy;

	do 
	{
		usIntTicksCpy = usIntTicks;
		now = AT91C_BASE_TC1->TC_CV;
	} while (usIntTicksCpy != usIntTicks);

	return usIntTicksCpy + (now + (kTimerTicksPerUs / 2)) / kTimerTicksPerUs; 
}

void
tmrWaitUS(LongWord numUS)
{
	LongWord endTime = numUS + tmrGetUS();

	while (TM_LT(tmrGetUS(), endTime))
	{
	}

}

LongWord
tmrGetMS()
{
	register LongWord msIntTickCpy;
	
	do
	{
		msIntTickCpy = msIntTick;
	} while (msIntTickCpy != msIntTick);
	
	return msIntTickCpy;
}	

void
tmrWaitMS(LongWord numMS)
{
	LongWord endTime = numMS + tmrGetMS();

	while (TM_LT(tmrGetMS(), endTime))
	{
	}

}



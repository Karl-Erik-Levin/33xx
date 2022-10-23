/* rtcDrv.h - Dataton 33xx
**
** Real-time clock driver.
**
** Speed adjusment are calculated using a PID requlator.
** For information on PID regulators visit:
** http://www.embedded.com/2000/0010/0010feat3.htm
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\hwBoard.h"
#include "driver\rtcDrv.h"
#include "driver\tmrDrv.h"
//#include "driver\pwrDrv.h"

//---------------------------------DEFINES----------------------------------------
#define KRTT_WANTED_PERIOD_MS	(250)
#define KRTT_NOMIAL_PERIOD_TICK (8192)
#define KRTT_PID_PGAIN			(0.5)
#define KRTT_PID_IGAIN			(0.1)
#define KRTT_PID_DGAIN			(0.01)

//------------------------------GLOBAL VARIABLES----------------------------------
static long gAccErr;					// Accumulated error (integral)
static long gPrevPeriod;
static LongWord gStartTime;
static LongWord gBaseTime; 

static Boolean gRtcSleeping = false;

//--------------------------------PROTOTYPS---------------------------------------
void rtcDrvISREntry(void);
void rtcDrvISR(void);

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	DoRTCISR
 * Summary:		Real time driver interrupt routine. Perform PID-regulator 
 *              to keep RTC in sync
 *******************************************************************************/
void
rtcDrvISR(void)
{
	LongWord speed = KRTT_NOMIAL_PERIOD_TICK;
	register LongWord rtsrCpy = AT91C_BASE_RTTC->RTTC_RTSR;
	
 	if (rtsrCpy & AT91C_RTTC_ALMS && gRtcSleeping)
	{
		// POWD_SetWakeSource(EWUS_RealTimeClock);
	}
	
	if (rtsrCpy & AT91C_RTTC_RTTINC)
	{
		{
		register long currTime = tmrGetMS();
		register long period = DIFF(currTime , gStartTime);
		long error;

		if (period < (KRTT_WANTED_PERIOD_MS - 50) || period > (KRTT_WANTED_PERIOD_MS + 50))
			period = KRTT_WANTED_PERIOD_MS;

		{			
			double pTerm, iTerm, dTerm, cTerm;
			double speedFraction;

			error = KRTT_WANTED_PERIOD_MS - period;
			pTerm = error * KRTT_PID_PGAIN;
			dTerm = KRTT_PID_DGAIN * (period - gPrevPeriod);

			gAccErr += error;
			iTerm = gAccErr * KRTT_PID_IGAIN;
			
			cTerm = KRTT_WANTED_PERIOD_MS + pTerm + iTerm - dTerm;
			speedFraction = cTerm / KRTT_WANTED_PERIOD_MS;
			speedFraction = KRTT_NOMIAL_PERIOD_TICK * speedFraction;
			speed = (long)speedFraction ;
			AT91F_RTTSetPrescaler(AT91C_BASE_RTTC, speed);	
			
			TRACE_RTC(("rtcDrvISR: %d %d %d %d %d\r\n",
					AT91F_RTTReadValue(AT91C_BASE_RTTC),
					period, gAccErr, error, speed));
		}

		gPrevPeriod = period;
		gStartTime = currTime;
		}
	}
	
	/* End the interrupt in the AIC. */
    AT91C_BASE_AIC->AIC_EOICR = 0;
}

/*******************************************************************************
 * Function:	rtcInit
 * Summary:		 
 *******************************************************************************/
void
rtcInit(void)
{
	AT91F_RTTSetPrescaler(AT91C_BASE_RTTC, KRTT_NOMIAL_PERIOD_TICK);
	
	gRtcSleeping = false;
	gAccErr = 0;
	gPrevPeriod= KRTT_WANTED_PERIOD_MS;
	gStartTime = tmrGetMS();

	rtcSetRtValue(0);

	AT91F_RTTSetRttIncINT(AT91C_BASE_RTTC);
	
#if (FREERTOS_TIMER != PIT)
	// Config AIC for RTC interrupt
	AT91F_AIC_ConfigureIt(
			AT91C_BASE_AIC, 
			AT91C_ID_SYS, 
			4, 
			AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE, 
			rtcDrvISREntry); 
	AT91F_AIC_ClearIt(AT91C_BASE_AIC, AT91C_ID_SYS);	// Clear pending interrupt
	AT91F_AIC_EnableIt(AT91C_BASE_AIC, AT91C_ID_SYS);	// Enable interrupt via AIC
#endif
}

/*******************************************************************************
 * Function:	rtcSetRtValue
 * Summary:		 
 *******************************************************************************/
void
rtcSetRtValue(LongWord rtValue)
{
	gBaseTime = rtValue;
	
	AT91F_RTTRestart(AT91C_BASE_RTTC);
}

/*******************************************************************************
 * Function:	rtcMSToTicks
 * Summary:		 
 *******************************************************************************/
LongWord
rtcMSToTicks(LongWord ms)
{
	return (ms +  KRTT_WANTED_PERIOD_MS / 2) / KRTT_WANTED_PERIOD_MS;
}

/*******************************************************************************
 * Function:	rtcClose
 * Summary:		 
 *******************************************************************************/
void
rtcClose(void)
{
}

/*******************************************************************************
 * Function:	rtcGetRtValue
 * Summary:		 
 *******************************************************************************/
LongWord
rtcGetRtValue(void)
{
	return AT91F_RTTReadValue(AT91C_BASE_RTTC) + gBaseTime;
}

/*******************************************************************************
 * Function:	rtcSleep
 * Summary:		If parameter wakeInNumMs==0 no alarm will be setup
 *******************************************************************************/
void
rtcSleep(LongWord wakeInNumMs)
{
	AT91F_RTTClearRttIncINT(AT91C_BASE_RTTC);
	
	if (wakeInNumMs) {
		AT91F_RTTSetAlarmValue(AT91C_BASE_RTTC,
							   AT91F_RTTReadValue(AT91C_BASE_RTTC) + (wakeInNumMs) / KRTT_WANTED_PERIOD_MS);
		AT91F_RTTSetAlarmINT(AT91C_BASE_RTTC);
	}
	
	gRtcSleeping = true;
}

/*******************************************************************************
 * Function:	rtcWake
 * Summary:		 
 *******************************************************************************/
void
rtcWake(void)
{
	gRtcSleeping = false;
	
	AT91F_RTTSetRttIncINT(AT91C_BASE_RTTC);
	AT91F_RTTClearAlarmINT(AT91C_BASE_RTTC);
}



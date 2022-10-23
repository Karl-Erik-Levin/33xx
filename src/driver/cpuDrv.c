/* cpuDrv.c - Dataton 33xx
**
**	Deals with CPU (AT91SAM7S) specific stuff
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
#include "driver\cpuDrv.h"

//--------------------------------FUNCTIONS---------------------------------------
void
cpuForceWatchdogReset(void)
{
	while (1)
		;
}

/* Intialize the Watchdog (Enable and set period)
**
** Note (From Atmel errata):

** WDT: The Watchdog Timer May Lock the Device in a Reset State
** Under certain rare circumstances, if the Watchdog Timer is used with the Watchdog Reset
** enabled (WDRSTEN set at 1), the Watchdog Timer may lock the device in a reset state when
** the user restarts the watchdog (WDDRSTT). The only way to recover from this state is a poweron
** reset. The issue depends on the values of WDD and WDV in the WDT_MR register.
** Problem Fix/Workaround
** Two workarounds are possible.
** 1. Either do not use the Watchdog Timer with the Watchdog Reset enabled (WDRSTEN
** set at 1),
** 2. or set WDD to 0xFFF and in addition use only one of the following values for WDV:
** 0xFFF, 0xDFF, 0xBFF, 0x9FF, 0x7FF, 0x77F, 0x6FF, 0x67F, 0x5FF, 0x57F, 0x4FF, 0x47F,
** 0x3FF, 0x37F, 0x2FF, 0x27F, 0x1FF, 0x1BF, 0x17F, 0x13F, 0x0FF, 0x0DF, 0x0BF, 0x09F,
** 0x07F, 0x06F, 0x00F
**
** Use only atmel apporved values.
**
*/
void
cpuInitWatchDog(void)
{
	// Setup Watchdog period to 3.00 seconds (=0x2FF,  Max=0xFFF ==> 16 seconds)
	// and set Watchdog reset enable
		
	AT91C_BASE_WDTC->WDTC_WDMR= AT91C_WDTC_WDIDLEHLT | AT91C_WDTC_WDDBGHLT | AT91C_WDTC_WDRSTEN | 0x2ff << 16 | 0x2ff;
}

void
cpuKickWatchDog(void)
{
	AT91F_WDTRestart(AT91C_BASE_WDTC);
}

/* If we sleep in fullPowerSave mode we will not be able to
   wake from IR interrupt
*/
void
cpuSleep(Boolean fullPowerSave)
{
	unsigned int tmp;
	
	AT91PS_PMC pPMC = AT91C_BASE_PMC;
	AT91PS_VREG pVREG = AT91C_BASE_VREG;
	
	// Disable the USB Transceiver (*)
	pPMC->PMC_PCER=1<<11;
	AT91C_BASE_UDP->UDP_TXVC = AT91C_UDP_TXVDIS;
	pPMC->PMC_PCDR=1<<11;

	if (fullPowerSave) {
		// Switch the Master Clock (MCK) to Slow Clock
		tmp = pPMC->PMC_MCKR;
		tmp &= ~AT91C_PMC_CSS;
		tmp |= AT91C_PMC_CSS_SLOW_CLK;
		pPMC->PMC_MCKR =tmp ;
		while(!(pPMC->PMC_SR & AT91C_PMC_MCKRDY))
			;
		
		// Disable the PLL
		pPMC->PMC_PLLR = 0x0;
	}
	
	// Disable the Main Oscillator
	pPMC->PMC_MOR = 0x0;
	
	// Put the Voltage Regulator in Standby Mode
	pVREG->VREG_MR = 0x1;
	
	// Reduce the MCK Frequency Down to 500 Hz
/*	tmp = pPMC->PMC_MCKR;
	tmp &= ~AT91C_PMC_PRES;
	tmp |= AT91C_PMC_PRES_CLK_64;
	pPMC->PMC_MCKR = tmp;
	while(!(pPMC->PMC_SR & AT91C_PMC_MCKRDY));		// */
	/* At this stage, PCK and MCK are running from the slow clock oscillator at 500Hz */
	
	
	/* Enter the processor in Idle Mode - Disable Processor Clock */
	pPMC->PMC_SCDR = AT91C_PMC_PCK | AT91C_PMC_UDP;
	/* At this stage, MCK is running from the slow clock oscillator at 500Hz */
	/* The PCK is disabled and waiting for an interrupt */

	// Speed up the MCK Frequency to slow clock
/*	tmp = pPMC->PMC_MCKR;
	tmp &= ~AT91C_PMC_PRES;
	tmp |= AT91C_PMC_PRES_CLK_2;
	pPMC->PMC_MCKR = tmp ;
	while(!(pPMC->PMC_SR & AT91C_PMC_MCKRDY));		// */

	// Put the Voltage Regulator in Normal Mode
	pVREG->VREG_MR = 0x0;
	
	// Enable the Main Oscillator
	pPMC->PMC_MOR = ((AT91C_CKGR_OSCOUNT & (0x06 <<8) | AT91C_CKGR_MOSCEN ));	
	while(!(pPMC->PMC_SR & AT91C_PMC_MOSCS))
		;
	
	if (fullPowerSave) {
		// Enable PLL
		pPMC->PMC_PLLR = ((AT91C_CKGR_DIV & 0x05) |
						  (AT91C_CKGR_PLLCOUNT & (28<<8)) |
						  (AT91C_CKGR_MUL & (25<<16)));		// 95,8464 =   18,432 / 5 * (25 + 1)
		while(!(pPMC->PMC_SR & AT91C_PMC_LOCK))
			;
		while(!(pPMC->PMC_SR & AT91C_PMC_MCKRDY))
			;
		
		// Processor Clock select the PLL clock
		pPMC->PMC_MCKR |= AT91C_PMC_CSS_PLL_CLK;
		while(!(pPMC->PMC_SR & AT91C_PMC_MCKRDY))
			;
	}
}


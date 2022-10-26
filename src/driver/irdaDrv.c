/* irdaDrv.c - Dataton 33xx
**
** IR Communication Driver.
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"

#include "driver\hwBoard.h"
#include "driver\irdaDrv.h"

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\utilities\fifo.h"
#include "platform\utilities\guideProtcol.h"

//---------------------------------DEFINES----------------------------------------
#define IRDA_BAUD_RATE			9600
#define IRDA_IRQ_PRIO			4

//---------------------------------TYPEDEFS---------------------------------------
typedef struct _irdaTrans
{
	Byte	msgSize;				// Messages size
	Byte	currPos;				// Current position in buf
	Byte	buf[IR_MESSAGE_SIZE];	// Buffer to hold messages
	Word	dataBits;				// Hold byte during transmit
	Boolean irPwr;					// IrDA transmission power, true=high
} irdaTrans; 

//------------------------------GLOBAL VARIABLES----------------------------------
static IrDaEventFunc receiveCB = NULL;
static Boolean	irdaSleeping = false; 

static irdaTrans rx;				// Receive transaction
static irdaTrans tx;				// Transmit transaction

/* This table contains the relative values to program register A and C with
** to generate waveforms for "0" and "1" in IrDa.
**
** Relative relates to relative to value of register C (The IRQ generating register).
*/
static Word gBitPatterns[2][2] =
{	/* RA		RC */
	{77*6, 		102*6},		// "0", rising -> falling			
	{204*6, 	102*6}		// "1", no rising -> just a c timer.
};

//--------------------------------PROTOTYPES---------------------------------------
extern void irdaDrvRxISREntry();
extern void irdaDrvTxISREntry();
void irdaDrvRxISR();
void irdaDrvTxISR();

//--------------------------------FUNCTIONS---------------------------------------
static void irdaDisableInt(void)
{
	AT91F_AIC_DisableIt(AT91C_BASE_AIC, AT91C_ID_US0);
	AT91F_AIC_DisableIt(AT91C_BASE_AIC, AT91C_ID_TC0);
}

static void irdaEnableInt(void)
{
	AT91F_AIC_EnableIt(AT91C_BASE_AIC, AT91C_ID_US0);
	AT91F_AIC_EnableIt(AT91C_BASE_AIC, AT91C_ID_TC0);
}

static void irdaEnableTXBoost(void)
{
#if (HARDWARE==3397)
	AT91F_PIO_ClearOutput(AT91C_BASE_PIOA, IRDA_BOOST_PIN);		// Set low, activate
#elif (HARDWARE==3356)
	AT91F_PIO_ClearOutput(AT91C_BASE_PIOA, IRDA_BOOST_PIN);		// Set low,  enable IR Boost
	AT91F_PIO_SetOutput(AT91C_BASE_PIOA, IRDA_SD_PIN);		// Set high, deactivate IrDA circuit
#endif
}

static void irdaDisableTXBoost(void)
{
#if (HARDWARE==3397)
	AT91F_PIO_SetOutput(AT91C_BASE_PIOA, IRDA_BOOST_PIN);		// Set high, deactivate
#elif (HARDWARE==3356)
	AT91C_BASE_PIOA->PIO_PER = IRDA_BOOST_PIN;
	AT91C_BASE_PIOA->PIO_OER = IRDA_BOOST_PIN;
	AT91C_BASE_PIOA->PIO_MDER =IRDA_BOOST_PIN;
	AT91C_BASE_PIOA->PIO_PPUER=IRDA_BOOST_PIN;
	
	AT91F_PIO_SetOutput(AT91C_BASE_PIOA, IRDA_BOOST_PIN);		// Set high, disable IR Boost
	AT91F_PIO_ClearOutput(AT91C_BASE_PIOA, IRDA_SD_PIN);		// Set low, activate IrDA circuit
#endif
}

static void
irdaEnableTX(Boolean fromISR)
{
	if (!fromISR)
		portENTER_CRITICAL();

	// Disable RX
	AT91C_BASE_US0->US_CR = AT91C_US_RXDIS;
	AT91C_BASE_US0->US_IDR = AT91C_US_RXRDY | AT91C_US_TIMEOUT;

	// Flush the reciver.
	while (AT91C_BASE_US0->US_CSR & AT91C_US_RXRDY ) {
		Byte t = AT91C_BASE_US0->US_RHR;
	}

	tx.currPos = 0;
	tx.dataBits = 0;
	
	if (tx.irPwr)
		irdaEnableTXBoost();
	
	AT91C_BASE_TC0->TC_CMR &= ~(1 << 6);		// CPCSTOP=0, Counter clock is not stopped when reaches RC

	AT91C_BASE_TC0->TC_CV = 0;					// Counter Value
	AT91C_BASE_TC0->TC_RA = 200;				// Register A
	AT91C_BASE_TC0->TC_RC = 100;				// Register C

	AT91C_BASE_TC0->TC_IER = AT91C_TC_CPCS;		// Interrupt Enable: Register C Compare
	AT91C_BASE_TC0->TC_CCR = AT91C_TC_SWTRG;	// (TC) Software Trigger Command

	if (!fromISR)
		portEXIT_CRITICAL();
}

static void
irdaEnableRX()
{
	rx.currPos = 0;
	rx.msgSize = IR_MESSAGE_SIZE;
	
	if (receiveCB) {
		// Flush Rx
		while (AT91C_BASE_US0->US_CSR & AT91C_US_RXRDY ) {
			Byte t = AT91C_BASE_US0->US_RHR;
		}
		
		// Enable Rx and reset timeout
		AT91C_BASE_US0->US_CR = AT91C_US_RXEN | AT91C_US_STTTO;
		
		// Enable interrupt
		AT91C_BASE_US0->US_IER = AT91C_US_RXRDY | AT91C_US_TIMEOUT;
	}
}

/* Receive IrDa interrupt service routine. */
#pragma ghs nothumb
void
irdaDrvRxISR()
{
	LongWord irqStatus = AT91C_BASE_US0->US_CSR & AT91C_BASE_US0->US_IMR;
	Boolean taskWoken = false;

	if (irdaSleeping) {
//		POWD_SetWakeSource(EWUS_IrDa);
	}

	if (irqStatus & (AT91C_US_RXRDY | AT91C_US_TIMEOUT))
	{	/* Have received one byte or timeout
		**
		** At this stage the transmitter is disabled.
	    */
		Byte irByte = AT91C_BASE_US0->US_RHR;

		if (irqStatus & AT91C_US_RXRDY) {
			rx.buf[rx.currPos] = irByte;
		}
		
		/* Check for...
		** (1) Buffer has been filled...
		** (2) Received a stop byte...
		** (3) Timeout
		** All these will complete/close the current read request.
		*/
		if ((++rx.currPos) >= rx.msgSize ||
			irByte == kEOF ||
			irqStatus & AT91C_US_TIMEOUT)
		{
			/* Read request has been completed.
			** (1) Notify upper tier.
			** (2) Check for pending write requests.
			*/
			if (receiveCB)
				taskWoken = receiveCB(rx.currPos, rx.buf);
			
			rx.currPos = 0;			// Prepare for next messages
			
			if (tx.msgSize != 0) {	// We got data to send, prioritize TX over RX
				irdaEnableTX(true);
				
			} else {			// Reset timeout
			
				AT91C_BASE_US0->US_CR = AT91C_US_STTTO;
			}
		}
	}

//	portEND_SWITCHING_ISR(taskWoken);
	
	/* End the interrupt in the AIC. */
	AT91C_BASE_AIC->AIC_EOICR = 0;
}

void
irdaDrvTxISR()
{
	LongWord sr = AT91C_BASE_TC0->TC_SR;

	if (!tx.dataBits) {
		if (tx.currPos < tx.msgSize) {	
			//            Stop BIT   DATABITS                    Start BIT
			tx.dataBits = (1 << 9) | (tx.buf[tx.currPos] << 1) | (0 << 0);
			tx.currPos++;
		}
	}
	
	{	// Feed next bit's timer values into RA and RC.
		register Word patternIx = tx.dataBits & 1;
		AT91C_BASE_TC0->TC_RA = AT91C_BASE_TC0->TC_RC + gBitPatterns[patternIx][0];
		AT91C_BASE_TC0->TC_RC = AT91C_BASE_TC0->TC_RC + gBitPatterns[patternIx][1];
		tx.dataBits >>= 1;			// Next bit
	}

	if (!tx.dataBits && tx.currPos >= tx.msgSize) {
		// Waveform for the last bit of last byte has been generated.

		irdaDisableTXBoost();
		tx.msgSize = 0;			// Messages completed

		// Stop timer upon next register C trigger.
		AT91C_BASE_TC0->TC_CMR |= AT91C_TC_CPCSTOP;		// CPCSTOP=1, Counter Clock Stopped with RC Compare
		AT91C_BASE_TC0->TC_IDR = AT91C_TC_CPCS;			// Interrupt Disable: RC Compare

		irdaEnableRX();
	}
	
	portEND_SWITCHING_ISR(pdFALSE);

	/* End the interrupt in the AIC. */
	AT91C_BASE_AIC->AIC_EOICR = 0;
}

void
irdaInit(IrDaEventFunc msgRcvCB)
{
	LongWord idx;
	
	tx.msgSize = 0;
	receiveCB = msgRcvCB;

	/****************************************************
	** Configure US0 for IRDA reception. 
	****************************************************/
	AT91F_PIO_CfgInput(AT91C_BASE_PIOA, IRDA_RX_PIN);
	AT91C_BASE_PIOA->PIO_PPUER = IRDA_RX_PIN;			// Disable pullup
	
	AT91F_AIC_DisableIt(AT91C_BASE_AIC, AT91C_ID_US0);
	
	AT91F_PMC_EnablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_US0);

	// Manual control of power boost pin
	AT91C_BASE_PIOA->PIO_PER = IRDA_BOOST_PIN;			// IR Boost pin in PIO mode
	AT91C_BASE_PIOA->PIO_OER = IRDA_BOOST_PIN;			// IR Boost pin configure as output
	AT91C_BASE_PIOA->PIO_MDER =IRDA_BOOST_PIN;			// IR Boost pin configure in open drain mode
	AT91C_BASE_PIOA->PIO_PPUER=IRDA_BOOST_PIN;			// IR Boost pin enable internal pullup
	irdaDisableTXBoost();								// Disable boost signal also enable IrDA circuit

    // Assign control of US0 RX pin to US0.
	AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA, IRDA_RX_PIN, 0);

	
	// Reset US0
	AT91C_BASE_US0->US_RTOR = 30;						// Receiver Timeout Register
	AT91C_BASE_US0->US_CR = AT91C_US_RSTRX |			// Reset Receiver
							AT91C_US_RSTSTA;			// Reset Status Bits
		
	// Setup US0 for IRDA
    AT91F_US_Configure(
			AT91C_BASE_US0,  
			MCK,
			AT91C_US_ASYNC_IRDA_MODE,
			IRDA_BAUD_RATE,    		              
			0);                     	           		// Timeguard to be programmed

	// Configure US0 interrupt.
	AT91F_AIC_ConfigureIt(
				AT91C_BASE_AIC, 
				AT91C_ID_US0, 
				IRDA_IRQ_PRIO, 
				AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE, 
				irdaDrvRxISREntry); 

	AT91F_AIC_EnableIt(AT91C_BASE_AIC, AT91C_ID_US0);
	
	/**************************************************************
	** Set up timer 0 to generate IrDa bit waveforms.
	**
	** Register A trigger is used to generate the rising edge,
	** Register C triggers the falling edge and generates the IRQ.
	** 
	**************************************************************/

	// Assign control of PA0 to timer.
	AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA, 0, IRDA_TX_PIN);	// PA0 = TIOA0
	AT91C_BASE_PIOA->PIO_PPUER = IRDA_TX_PIN;				// Disable pullup
	
	AT91F_AIC_DisableIt(AT91C_BASE_AIC, AT91C_ID_TC0);
	AT91F_PMC_EnablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_TC0);
	AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKDIS;
	AT91C_BASE_TC0->TC_IDR = 0xffffffff;

	// Should be globally defined!						?? FJ comment
	// All timers clocked through internal oscilator.	?? FJ comment
	AT91C_BASE_TCB->TCB_BMR = AT91C_TCB_TC2XC2S_NONE |	// (TCB) None signal connected to XC2
							  AT91C_TCB_TC1XC1S_NONE |	// (TCB) None signal connected to XC1
							  AT91C_TCB_TC0XC0S_NONE;	// (TCB) None signal connected to XC0
	
	idx = AT91C_BASE_TC0->TC_SR;	

	AT91F_AIC_ConfigureIt(
			AT91C_BASE_AIC, 
			AT91C_ID_TC0,
			IRDA_IRQ_PRIO, 
			AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE,
			irdaDrvTxISREntry);
	
	AT91F_AIC_ClearIt(AT91C_BASE_AIC, AT91C_ID_TC0);
	AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKEN;				// Enable clock, but do NOT start it.
	AT91F_AIC_EnableIt(AT91C_BASE_AIC, AT91C_ID_TC0);
			
	AT91C_BASE_TC0->TC_CMR =
		1 << 15 |			// WAVE  = 1, Waveform mode is enabled
		0 << 13 |			// WAVSEL= 00 Incrementing, reset at 0xffff.
		1 << 16 |			// ACPA: RA compare effect on TIOA ==> Set
		2 << 18 |			// ACPC: RC compare effect on TIOA ==> Clear
		AT91C_TC_CLKS_TIMER_DIV2_CLOCK;

	// Enable RX if we have a callback function
	irdaEnableRX();
}

void
irdaClose()
{
	AT91F_AIC_DisableIt(AT91C_BASE_AIC, AT91C_ID_US0);
	AT91F_PDC_DisableTx(AT91C_BASE_PDC_US0);
	
	AT91F_PMC_DisablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_US0);
	AT91F_PMC_DisablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_TC0);
}

Boolean
irdaSend(Byte *msg, Byte msgSize, Boolean irPwr) {

	if (tx.msgSize != 0)
		return false;				// Busy, already sending a messages.
	
	memcpy(tx.buf, msg, msgSize);	// Make a copy of messages to send
	tx.msgSize = msgSize;
	tx.currPos = 0;
	tx.dataBits= 0;
	tx.irPwr   = irPwr;
		
//	irdaDisableInt();
	if (rx.currPos == 0) {			// No read in progress
		irdaEnableTX(false);
	}
//	irdaEnableInt();

	return true;
}

void
irdaSleep(void)
{
	AT91F_PMC_DisablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_US0);
	AT91F_PMC_DisablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_TC0);

	irdaSleeping = true;
}

void
irdaWake(void)
{
	irdaSleeping = false;

	AT91F_PMC_EnablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_US0);
	AT91F_PMC_EnablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_TC0);
}	


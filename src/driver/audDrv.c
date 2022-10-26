/* audDrv.c - Dataton 3356 Pickup
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
** Dependencies:
** - I2C driver, used for the control interface of the MAX9850. (Used for settings, volume, data rate, etc)
** - SCC I/O module, used to transfer audio data to MAX9850.
** - US1 I/O module, the uart baudrate generator is used to generate master clock to MAX9850.
**
** Created  09-09-24	Kalle
** Modifyed 22-09-06    Kalle	Implement radio support
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\hwBoard.h"
#include "driver\audDrv.h"
#include "driver\i2cDrv.h"

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"

//---------------------------------DEFINES----------------------------------------
//#define FREDJ
#define MAX9850_VOLUME_MAX 40					// Number of attenuation steps in MAX9850

#define SCC_INTERRUPTS		(AT91C_SSC_TXBUFE | AT91C_SSC_ENDTX)
#define SCC_IRQ_PRIO		4

#define TWO_RAISED_BY_22	4194304
#define MAX_9850_MCK		12000000			// 12 Mhz, Frequency feed to the 9850

enum {							// 9850 register ID's
    MAX9850_Status_A = 0,
    MAX9850_Status_B,
    MAX9850_Volume,
    MAX9850_General_Purpose,
    MAX9850_IRQ_Enable,
    MAX9850_Enable,
    MAX9850_Clock,
    MAX9850_Charge_Pump,
    MAX9850_LRCK_MSB,
    MAX9850_LRCK_LSB,
    MAX9850_Digital_Audio
};
// Bits in MAX9850 registers
//	MAX9850_Volume
#define DA_MUTE				0x80				// Mute
#define DA_SLEW				0x40				// Slew rate control enable
//	MAX9850_General_Purpose
#define DA_GPIO_ISOUT		0x20				// GPIO direction is out
#define DA_MONO				0x04				// Amplifier in MONO mode
#define DA_ZDEN				0x01				// Zero-detect enable
//	MAX9850_Charge_Pump
#define DA_SR_42MS			0xc0				// Volume slew rate 42 ms
#define DA_SR_63MS			0x80				// Volume slew rate 63 ms
#define DA_SR_125MS			0x40				// Volume slew rate 125 ms
#define DA_SR_63US			0x00				// Volume slew rate 63 us
//	MAX9850_Digital_Audio
#define	DA_MAS				0x80				// Master mode
#define	DA_LRCLK_INV		0x40				// LR Clk invert
#define DA_BCLK_INV			0x20				// Bit Clk invert
#define DA_LSF				0x10				// Least significant bit first
#define	DA_DLY				0x08				// SDIN delay one bit
#define	DA_RTJ				0x04				// Right justified data
#define	DA_24B				0x03				// Audio data 24 bits
#define	DA_20B				0x02				// Audio data 20 bits
#define	DA_18B				0x01				// Audio data 18 bits
#define	DA_16B				0x00				// Audio data 16 bits

//------------------------------GLOBAL VARIABLES----------------------------------
static const Byte gVolumeMapping[] =
{
	 0,  4,  6,  7,	 8,  9, 10,	11, 12, 13,
	14, 15,	16, 17, 18, 19, 20, 21,	22, 23,
	24, 25,	26, 27, 28, 29, 30, 31,	32, 33,
	34, 35,	36, 36, 37, 37, 38, 38, 39, 39,
	40
};

static LongWord gPlayBackRate;
static Boolean	gRadioAudioReq = false;
static Byte		gADUsage = 0;
static Byte		volumeRegShadow;
static Boolean  audSleeping;

//--------------------------------PROTOTYPS---------------------------------------
extern void audDrvISREntry();
void audDrvISR(void);

//--------------------------------FUNCTIONS---------------------------------------
#pragma ghs nothumb
/*******************************************************************************
 * Function:	audDrvISR
 *
 * Summary:		SSC interrupt service routine
 *******************************************************************************/
void
audDrvISR()
{
	if (AT91F_PDC_IsNextTxEmpty(AT91C_BASE_PDC_SSC)) {
		// If we have a queue of buffers to send it will be here to send next
		//	AT91F_PDC_SetNextTx(AT91C_BASE_PDC_SSC, (char *)req->sampleData, req->numSamples);
	}

	if (AT91F_PDC_IsTxEmpty(AT91C_BASE_PDC_SSC)) {
		AT91F_SSC_DisableIt(AT91C_BASE_SSC, SCC_INTERRUPTS);
	}
	
	TRACE_AUD((".");)
	
	/* End the interrupt in the AIC */
	AT91C_BASE_AIC->AIC_EOICR = 0;
}

/*******************************************************************************
 * Function:	startMCK
 *
 * Summary:		Setup master clock for the 9850
 *******************************************************************************/
static void
startMCK()
{
    AT91F_US1_CfgPMC();				// Feed clock to UART.

	// Feed SCK1 to 9850. That is the baudrate clock of the US1, and is used as the MCLK for the 9850.
	AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA, I2S_MCLK_PIN, 0);
	AT91C_BASE_PIOA->PIO_PPUDR = I2S_MCLK_PIN;				// Diasable pullup

    // Configure US1 to generate master clock signal to the MAX9850.
    AT91C_BASE_US1->US_CR = AT91C_US_RSTTX;         
    AT91C_BASE_US1->US_MR = AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK | AT91C_US_CKLO | AT91C_US_SYNC;
    AT91C_BASE_US1->US_BRGR = 4;	// MCK/4 ~= 12 MHz
    AT91C_BASE_US1->US_CR = AT91C_US_TXEN;
}

/*******************************************************************************
 * Function:	sccInit
 *
 * Summary:		Initialize SCC interface to talk to the MAX 9850
 *******************************************************************************/
static void
sccInit()
{
    AT91F_SSC_CfgPMC();         // Feed clock to SSC.
	
	// Enabe pheriphal function of SCC I/O pins(output only).	    
    AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA, SCC_PINS_MASK, 0);
//	AT91C_BASE_PIOA->PIO_PPUDR = SCC_PINS_MASK;					// Diasable pullup

    // Configure SSC for I2S audio transmission.
    AT91C_BASE_SSC->SSC_CR = AT91C_SSC_SWRST;	// Perform software reset
    AT91F_PDC_Close(AT91C_BASE_PDC_SSC);		// Disable PDC TX and RX. Reset PDC transfer descriptors

#ifdef FREDJ
    AT91C_BASE_SSC->SSC_TCMR = 2/*CKS_TK*/ | 6 << 8 |
		((1 << 16) & AT91C_SSC_STTDLY); /*|
										 (15 << 24);*/

    AT91C_BASE_SSC->SSC_TFMR = (16 - 1) | AT91C_SSC_MSBF | (((2-1) << 8) & AT91C_SSC_DATNB);

	AT91C_BASE_SSC->SSC_TCMR  = 0x10722;
	AT91C_BASE_SSC->SSC_TFMR  = 0x0f;
#else
	AT91C_BASE_SSC->SSC_TCMR  = 0x00522;		// SSC Transmit Clock Mode Register (0x10722)
		// CKS[0:1] Transmit Clock Selection 			= 2 TK Pin
		// CKO[2:4] Transmit Clock Output Mode Selection= 0 none (input-only)
		// CKI[5]   Transmit Clock Inversion			= 1 The Frame sync signal input is sampled on Transmit clock falling edge.	
		// CKG[6:7]  Transmit Clock Gating Selection	= 0 None, continuous clock
		// START[8:11] Transmit Start Selection			= 5 Start farme at  rising edge on TF signal
		// STTDLY[16:23] Transmit Start Delay			= 0
		// PERIOD[24:31] Transmit Period Div. Selection = 0 ???
			
	AT91C_BASE_SSC->SSC_TFMR  = 0x010F;			// SSC Transmit Frame Mode Register (0x000F)
		// DATLEN[00:04]	Data Length							= 0xF 16-bit
		// DATDEF[05:05]	Data Default Value					= 0x0 Low level when not used
		// MSBF[07:07]		Most Significant Bit First			= 0x0 LSB first
		// DATNB[08:11]		Data Number per frame				= 0x1 2 data word
		// FSLEN[16:19]		Transmit Frame Sync Length			= 0x0 Not used
		// FSOS[20:22]		Transmit Frame Sync Output Selection= 0x0 Not used
		// FSDEN[23:23]		Frame Sync Data Enable				= 0x0 ???
		// FSEDGE[24:24]	Frame Sync Edge Detection			= 0x0 Pos edge
#endif

	// Configure SSC interrupt.
	AT91F_AIC_ConfigureIt(
				AT91C_BASE_AIC, 
				AT91C_ID_SSC, 
				SCC_IRQ_PRIO, 
				AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE, 
				audDrvISREntry); 
} 

/*******************************************************************************
 * Function:	twiWrite
 *
 * Summary:		Write a byte to a register at the 9850
 *******************************************************************************/
static void
twiWrite(
    Byte reg,            // The register to write to
    Byte value           // The value to write to the register. 
) {
	i2cWrite(EI2CD_9850, reg, &value, 1);
}

/*******************************************************************************
 * Function:	audInitInt
 * Summary:		
 *******************************************************************************/
static void
audInitInt()
{
	Byte enableReg;
	
	volumeRegShadow |= DA_SLEW;						// ENABLE_VOLUME_SKEW, Allow smooth volume changed.
	audSpeakerAmp(false);							// Disable speaker.
	
    startMCK();										// Enable 12 MHz MCLK to MAX9850
	
	twiWrite(MAX9850_Enable, 0x00);    				// Powerdown
#ifdef FREDJ
	twiWrite(MAX9850_Digital_Audio, 0x80 + 0x00 + 0x10 + 0x4 + 0x01);   // Master, 18 bit data, I2S

	twiWrite(MAX9850_LRCK_LSB, 0x50);  
	twiWrite(MAX9850_LRCK_MSB, 0x3c);   			// Integer mode

	twiWrite(MAX9850_Charge_Pump, 0x09);         	// Charge pump for 12 MHz operation
	twiWrite(MAX9850_Clock, 0 << 2);             	// SF = IC(1:0)+1 (Divide clock with 1)
#else
    twiWrite(MAX9850_Volume, 1 << 6 | 0x3f);		// Unmute, Enable slew, volume min
	twiWrite(MAX9850_Digital_Audio, 0x80 + 0x20 + 0x10 + 0x00 + 0x00);   // Master, MSB, 16 bit data, BCLK neg

	twiWrite(MAX9850_LRCK_LSB, 0x36);				// Samplerate 44100 Hz
	twiWrite(MAX9850_LRCK_MSB, 0x3c);   			// None integer mode

	twiWrite(MAX9850_Charge_Pump, 0x09);         	// Charge pump for 12 MHz operation
	twiWrite(MAX9850_Clock, 0 << 2);             	// SF = IC(1:0)+1 (Divide clock with 1)
#endif
	
	sccInit();										// Prepare SCC interface to send audio data

	// Initlize the SSC PDC.
	AT91F_PDC_SetNextTx(AT91C_BASE_PDC_SSC, (char *)NULL, 0);
	AT91F_PDC_SetTx(AT91C_BASE_PDC_SSC, (char *)NULL, 0);
	AT91F_PDC_EnableTx(AT91C_BASE_PDC_SSC);
	
	AT91F_SSC_EnableTx(AT91C_BASE_SSC);

	twiWrite(MAX9850_General_Purpose, 0x01);		// Enabled ZDEN
	vTaskDelay(20);

	enableReg = gRadioAudioReq ? 0xFF : 0xFD;		// Enable linein or not
	twiWrite(MAX9850_Enable, enableReg);			// Enable DAC, power, clock, charge pump, etc.
	vTaskDelay(40);
}

/*******************************************************************************
 * Function:	audInit
 * Summary:		
 *******************************************************************************/
void
audInit()
{
	volumeRegShadow	= 0;
	audSleeping     = false;
	gPlayBackRate   = AD_PLAYBACK_RATE_MIN;
	gADUsage		= 0;
	
	audInitInt();
	twiWrite(MAX9850_Volume, 0);					// Volume off
}

/*******************************************************************************
 * Function:	audClose
 * Summary:		
 *******************************************************************************/
void
audClose()
{
	audSpeakerAmp(false);
	audSleep();
}

/*******************************************************************************
 * Function:	audSpeakerAmp
 *
 * Summary:		Enable or disable speaker amplifier
 *******************************************************************************/
void
audSpeakerAmp(Boolean enable)
{
	AT91F_PIO_CfgOutput(AT91C_BASE_PIOA, AD_SPEAKER_CTRL_PIN);
	
	if (enable)
		AT91F_PIO_SetOutput(AT91C_BASE_PIOA, AD_SPEAKER_CTRL_PIN);
	else
		AT91F_PIO_ClearOutput(AT91C_BASE_PIOA, AD_SPEAKER_CTRL_PIN);
}

/*******************************************************************************
 * Function:	audSetMute
 * Summary:		
 *******************************************************************************/
void 
audSetMute(Boolean mute)
{
	if (mute)
		volumeRegShadow |= DA_MUTE;
	else
		volumeRegShadow &=~DA_MUTE;
}

/*******************************************************************************
 * Function:	audSetVolume
 * Summary:		
 *******************************************************************************/
void 
audSetVolume(Word volume)
{
	Word sendVolume;
	
	if (volume > AD_VOLUME_MAX)
		volume  = AD_VOLUME_MAX;

	sendVolume = MAX9850_VOLUME_MAX - gVolumeMapping[volume * MAX9850_VOLUME_MAX / AD_VOLUME_MAX];
	volumeRegShadow  = (volumeRegShadow & (DA_MUTE | DA_SLEW)) | sendVolume;

	if (!audSleeping)
		twiWrite(MAX9850_Volume, volumeRegShadow);
}

/*******************************************************************************
 * Function:	audSetPlaybackRate
 * Summary:		
 *******************************************************************************/
void 
audSetPlaybackRate(Word samplesPerSec)
{
	gPlayBackRate  = samplesPerSec;
	
	{
		long long sf = samplesPerSec;
		long long c = TWO_RAISED_BY_22;
		long long m = c * sf;
		LongWord rateDiv = (m + (MAX_9850_MCK/2)) / MAX_9850_MCK;

		twiWrite(MAX9850_LRCK_LSB, rateDiv & 0xff);  
		twiWrite(MAX9850_LRCK_MSB, (rateDiv  >> 8) & 0xff);
	}			
}

/*******************************************************************************
 * Function:	audSleep
 * Summary:		
 *******************************************************************************/
void
audSleep()
{
	if (!audSleeping) {
		audSleeping = true;
		
		/* Wait for playback to finish */
		while (!AT91F_PDC_IsTxEmpty(AT91C_BASE_PDC_SSC))
			;
	
		twiWrite(MAX9850_Volume, 0x3f);					// Volume to off
		twiWrite(MAX9850_Enable, 0x00);					// Power down MAX9850
		
		/* Disable clock to all audio I/O modules */
		AT91F_PMC_DisablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_SSC);	// Stop SSC
		AT91F_PMC_DisablePeriphClock(AT91C_BASE_PMC, 1 << AT91C_ID_US1);	// Stop MCLK
	}
}

/*******************************************************************************
 * Function:	audWake
 * Summary:		
 *******************************************************************************/
void
audWake()
{
	if (audSleeping) {
		audSleeping = false;
		
		audInitInt();
		twiWrite(MAX9850_Volume, volumeRegShadow);
  
		audSetPlaybackRate(gPlayBackRate);
	}
}

/*******************************************************************************
 * Function:	audGetBufferCount
 *
 * Summary:		Calculate the number of samples waiting to be transfered
 *				to the MAX 9850
 *******************************************************************************/
LongWord
audGetBufferCount()
{
	LongWord r;
	LongWord d1, d2;
	
    portENTER_CRITICAL();

	do {
		d1 = AT91C_BASE_PDC_SSC->PDC_TNCR;
		d2 = AT91C_BASE_PDC_SSC->PDC_TCR;
	} while (AT91C_BASE_PDC_SSC->PDC_TNCR != d1 || AT91C_BASE_PDC_SSC->PDC_TCR != d2);
	
    portEXIT_CRITICAL();

	r = d1 + d2;
	return r;
}

/*******************************************************************************
 * Function:	audFlush
 * Summary:		
 *******************************************************************************/
void
audFlush()
{
   portENTER_CRITICAL();
   
   AT91F_PDC_SetNextTx(AT91C_BASE_PDC_SSC, (char *)NULL, 0);
   AT91F_PDC_SetTx(AT91C_BASE_PDC_SSC, (char *)NULL, 0);

   portEXIT_CRITICAL();
}

/*******************************************************************************
 * Function:	audPlayBuffer
 * Summary:		
 *******************************************************************************/
Boolean
audPlayBuffer(short *sampleData, LongWord numSamples)
{
	Boolean retStatus = false;
	LongWord numWord = numSamples * 2;
	
	portENTER_CRITICAL();
	
	// Check if second bank is empty
	if (AT91F_PDC_IsNextTxEmpty(AT91C_BASE_PDC_SSC)) {
	    
		AT91F_PDC_SetNextTx(AT91C_BASE_PDC_SSC, (char *)sampleData, numWord);
		retStatus = true;
	}
	
	portEXIT_CRITICAL();
	
	return retStatus;
}

/*******************************************************************************
 * Function:	audRegUser
 * Summary:		
 *******************************************************************************/
void
audRegUser(EADUser user)
{
	gADUsage |= 1 << user;
	if (gADUsage)
	{
		audWake();
	}
}

/*******************************************************************************
 * Function:	audUnregUser
 * Summary:		
 *******************************************************************************/
void
audUnregUser(EADUser user)
{
	gADUsage &= ~(1 << user);
	if (!gADUsage)
	{
		audSleep();
	}
}

/*******************************************************************************
 * Function:	audEnableRadio
 * Summary:		
 *******************************************************************************/
void
audEnableRadio(void)
{
	gRadioAudioReq = true;

	if (!audSleeping) {
		twiWrite(MAX9850_Enable, 0xFF);    		// Enable LIN
	}
}

/*******************************************************************************
 * Function:	audDisableRadio
 * Summary:		
 *******************************************************************************/
void
audDisableRadio(void)
{
	gRadioAudioReq = false;

	if (!audSleeping) {
		twiWrite(MAX9850_Enable, 0xFD);    		// Disable LIN
	}
}















//****************************************************************************
#if 0
Boolean
audPlayBuffer(short *sampleData, LongWord numSamples)
{
	if (AT91F_PDC_IsNextTxEmpty(AT91C_BASE_PDC_SSC))
	{
	    portENTER_CRITICAL();
	    
		AT91F_PDC_SetNextTx(AT91C_BASE_PDC_SSC, (char *)sampleData, numSamples);
//		AT91F_SSC_EnableIt(AT91C_BASE_SSC, SCC_INTERRUPTS);
//		AT91F_AIC_EnableIt(AT91C_BASE_AIC, AT91C_ID_SSC);
		AT91C_BASE_SSC->SSC_CR = AT91C_SSC_TXEN;
		
		portEXIT_CRITICAL();
		
		return true;
	}
	
	return false;
}
//------------------------------------------------------------------------------
/// Sends the contents of a data buffer a SSC peripheral, using the PDC. Returns
/// true if the buffer has been queued for transmission; otherwise returns
/// false.
/// \param ssc  Pointer to an AT91S_SSC instance.
/// \param buffer  Data buffer to send.
/// \param length  Size of the data buffer.
//------------------------------------------------------------------------------
unsigned char SSC_WriteBuffer(AT91S_SSC *ssc,
                                     void *buffer,
                                     unsigned int length)
{
    // Check if first bank is free
    if (ssc->SSC_TCR == 0) {

        ssc->SSC_TPR = (unsigned int) buffer;
        ssc->SSC_TCR = length;
        ssc->SSC_PTCR = AT91C_PDC_TXTEN;
        return 1;
    }
    // Check if second bank is free
    else if (ssc->SSC_TNCR == 0) {

        ssc->SSC_TNPR = (unsigned int) buffer;
        ssc->SSC_TNCR = length;
        return 1;
    }
      
    // No free banks
    return 0;
}
//------------------------------------------------------------------------------
/// Enables the transmitter of a SSC peripheral.
/// \param ssc  Pointer to an AT91S_SSC instance.
//------------------------------------------------------------------------------
void SSC_EnableTransmitter(AT91S_SSC *ssc)
{
    ssc->SSC_CR = AT91C_SSC_TXEN;
}

//------------------------------------------------------------------------------
/// Disables the transmitter of a SSC peripheral.
/// \param ssc  Pointer to an AT91S_SSC instance.
//------------------------------------------------------------------------------
void SSC_DisableTransmitter(AT91S_SSC *ssc)
{
    ssc->SSC_CR = AT91C_SSC_TXDIS;
}

#endif


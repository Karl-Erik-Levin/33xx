#include "SpiritMP3Decoder.h"

int flags;	// To make TracePrintf() happy

void __DIR();
void __DIR() {}

void __EIR();
void __EIR() {}

void AT91F_Spurious_handler(void);
void AT91F_Spurious_handler(void) {}

void AT91F_Default_IRQ_handler(void);
void AT91F_Default_IRQ_handler(void) {}

void AT91F_Default_FIQ_handler(void);
void AT91F_Default_FIQ_handler(void) {}

void vPortPreemptiveTick();
void vPortPreemptiveTick() {}

void vPortStartFirstTask();
void vPortStartFirstTask() {}

void usTimerISREntry(void);
void usTimerISREntry(void){}

// Instead of irdaDrvISR.arm
void irdaDrvRxISR();
void irdaDrvRxISREntry();
void irdaDrvRxISREntry()
{
	irdaDrvRxISR();
}

// Instead of irdaDrvISR.arm
void irdaDrvTxISR();
void irdaDrvTxISREntry();
void irdaDrvTxISREntry()
{
	irdaDrvTxISR();
}

// Instead of audDrvISR.arm
void audDrvISR();
void audDrvISREntry();
void audDrvISREntry()
{
	audDrvISR();
}

// Instead of pwrDrvISR.arm
void pwrDrvISR();
void pwrDrvISREntry();
void pwrDrvISREntry()
{
	pwrDrvISR();
}

// Instead of rtcDrvISR.arm
void rtcDrvISR();
void rtcDrvISREntry();
void rtcDrvISREntry()
{
	rtcDrvISR();
}

void usb_it_handler();
void usbISREntry(void);						// HCC
void usbISREntry(void)
{
	usb_it_handler();
}

/******************************************************************************
*
* MPEG-1,2,2.5 Layer 3 (mp3) decoder.
* (C) Spirit corp. Moscow 2000-2002.
*
*******************************************************************************/
void
SpiritMP3DecoderInit(
	TSpiritMP3Decoder *pDecoder,				// Decoder structure
   	fnSpiritMP3ReadCallback *pCallbackFn,		// Data reading callback function
	void *token,								// Optional parameter for callback function
	fnSpiritMP3ProcessCallback *fpProcess)		// Data processing callback function
{
}

unsigned int
SpiritMP3Decode (
    TSpiritMP3Decoder *pDecoder,	// Decoder structure
    short *pPCMSamples,				// [OUT] Output PCM buffer
    unsigned int nSamplesRequired,	// Number of samples to decode (1 sample = 32 bit = 2ch*16 bit)
    TSpiritMP3Info *pMP3Info)		// [OUT, opt] Optional informational structure
{
	return 0;
}


#if 0
void SPIISREntry(void);
void SPIISREntry(void){}

void USBISREntry(void);						// FredJ
void USBISREntry(void){}

void vPortInitialiseBlocks( void );
void vPortInitialiseBlocks( void ){}
#endif

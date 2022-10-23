/* hpcDrv.c - Dataton 33xx
**
**	Headphone conector Driver
** 		
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
**/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\hwBoard.h"
#include "driver\adcDrv.h"
#include "driver\hpcDrv.h"

#include "hcc\target.h"

#include "Platform\FreeRTOS\FreeRTOS.h"
#include "Platform\FreeRTOS\task.h"

//---------------------------------DEFINES----------------------------------------
#if (HARDWARE == 3356)

// Voltage at headphoneSense HEADPHONES:	1,32V
// 							 NONE:			1,60V
// 							 USBPWR:		2,34V
  #define HEADPHONES_CONNECTED_THRESHOLD	460		// 1,43 V
  #define USB_CONNECTED_THRESHOLD			600		// 1,93 V
  
#elif (HARDWARE == 3397)

  #define MIC_CONNECTED_THRESHOLD			300		// 1,
  #define USB_CONNECTED_THRESHOLD			600		// 1,93 V
  
#endif
//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	hpcInit
 * Returns:		void
 * 
 * Summary:		Initialize head phone connection to assume audio output
 *******************************************************************************/
void
hpcInit()
{
	// Config pins as PIO output
	AT91F_PIO_CfgOutput(AT91C_BASE_PIOA, AUD_USB_SWT_NEW_PIN);	
	AT91F_PIO_CfgOutput(AT91C_BASE_PIOA, AUD_USB_SWT_OLD_PIN);	
	AT91F_PIO_CfgOutput(AT91C_BASE_PIOA, USB_PULLUP_CONTROL_PIN);
	
	// Pullup disable
	AT91C_BASE_PIOA->PIO_PPUDR = (AUD_USB_SWT_NEW_PIN |
								  AUD_USB_SWT_OLD_PIN |
								  USB_PULLUP_CONTROL_PIN);
	
	hpcReleaseUSB();				// Default = Audio mode
	hpcActivateUSBPullup(false);	// Deactivate USB pullup
	
	vTaskDelay(250);				// Give adcDrv some time to get a valid 
									// EPOTDC_Headphones sample
}

/*******************************************************************************
 * Function:	hpcSense
 * Returns:		EHPConnection -enum
 * 
 * Summary:		Returns the current headphone connection
 *******************************************************************************/
EHPConnection 
hpcSense(void)
{
	EHPConnection retValue = EHPC_VOID;
	Word analogLevel = adcGetLevel(EADC_Headphones);
	static Word lastLevel=0;
	
	if (analogLevel > USB_CONNECTED_THRESHOLD) {
		retValue = EHPC_USB;
	}
	else {
	  #if (HARDWARE==3397)
		if (analogLevel < MIC_CONNECTED_THRESHOLD) {
			retValue = EHPC_MIC;
		}
	  #else	// HARDWARE==3356
		if (analogLevel < HEADPHONES_CONNECTED_THRESHOLD) {
			retValue = EHPC_EARPHONE;
		}
	  #endif
	}
	
	if (lastLevel != analogLevel) {
		lastLevel = analogLevel;
		TRACE_HPC(("hpcDrv: analogLevel=%d\r\n", analogLevel);)
	}
	
	return retValue;
}

/*******************************************************************************
 * Function:	hpcGoUSB
 * Returns:		void
 * 
 * Summary:		Goto into USB mode
 *******************************************************************************/
void
hpcGoUSB()
{
	AT91F_PIO_SetOutput(AT91C_BASE_PIOA,  AUD_USB_SWT_OLD_PIN);		// Switch to 
	AT91F_PIO_ClearOutput(AT91C_BASE_PIOA,AUD_USB_SWT_NEW_PIN);		// USB mode
}

/*******************************************************************************
 * Function:	hpcReleaseUSB
 * Returns:		void
 * 
 * Summary:		Release USB bus
 *******************************************************************************/
void
hpcReleaseUSB()
{
	AT91F_PIO_SetOutput(AT91C_BASE_PIOA,  AUD_USB_SWT_NEW_PIN);		// Switch to 
	AT91F_PIO_ClearOutput(AT91C_BASE_PIOA,AUD_USB_SWT_OLD_PIN);		// Audio mode
}

/*******************************************************************************
 * Function:	hpcActivateUSBPullup
 * Returns:		void
 * 
 * NOTE:		Used by HCC usb.c (USB driver) to controll pullup on USB+
 *				this will signal to host that we are ready to enumerate USB
 *******************************************************************************/
void hpcActivateUSBPullup(Boolean on)
{
  if (on)
  {
    /* Set PA24 pin level to low to activate pullup. */
    *AT91C_PIOA_CODR = USB_PULLUP_CONTROL_PIN;
  }
  else
  {
    /* Set PA24 pin level to high to deactivate pullup. */
    *AT91C_PIOA_SODR = USB_PULLUP_CONTROL_PIN;
  }
}

/*******************************************************************************
 * Function:	pup_on_off
 * Returns:		void
 * 
 * NOTE:		Used by HCC usb.c (USB driver) to controll pullup on USB+
 *				this will signal to host that we are ready to enumerate USB
 *******************************************************************************/
void pup_on_off(hcc_u8 on)
{
	hpcActivateUSBPullup(on);
}

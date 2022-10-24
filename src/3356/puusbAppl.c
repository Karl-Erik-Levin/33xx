/* puusbAppl.h - Dataton 3356 Pickup
**
**	USB application
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

#include "driver\hwBoard.h"					// Drivers
#include "driver\cpuDrv.h"
#include "driver\hpcDrv.h"
#include "driver\ledDrv.h"
#include "driver\rtcDrv.h"
#include "driver\tmrDrv.h"
#include "driver\hccusb\usb.h"

#include "platform\hccusb\simple_scsi.h"
#include "platform\hccusb\usb_mst.h"
#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"
#include "platform\evtMng.h"
#include "platform\ledMng.h"
#include "platform\pwrMng.h"

#include "3356\puusbAppl.h"
#include "3356\usbVenReqHdlr.h"

//---------------------------------DEFINES----------------------------------------
#define FLASH_TIME_AFTER_DATA   (1000 * 1)			// 1 second.
#define FLASH_TIME_AFTER_START  (1000 * 4)			// 4 seconds.
#define CHARGE_DELAY			(1000 * 10)			// 10 seconds.

//----------------------------GLOBAL VARIABLES------------------------------------
static LongWord gUSBRWEvtTime;
static LongWord gUSBStartTime;
static Boolean gDataWritten;
static Boolean gVRReset;					// From Vendor request messages
static Boolean gVRIndicateData;				// From Vendor request messages
static Boolean gVRChargePermitted;			// From Vendor request messages

//--------------------------------FUNCTIONS---------------------------------------

/*******************************************************************************
 * Function:	msCallback
 * Summary:		Will be called by mass storage for every READ10 and WRITE10 command
 *******************************************************************************/
static void
msCallback(hcc_u8 caller)
{
	switch (caller) {
	case STSCSI_READ_10:
	case STSCSI_WRITE_10:
		gUSBRWEvtTime = tmrGetMS();		// Current time.
		ledDisplay(kLedUsbData);
		
	    if (caller == STSCSI_WRITE_10) {
			gDataWritten = true;
		}
		break;
	}
}

static void
dispatchVREvent(EMEvent *evt)
{
	TRACE_USB(("VREvent: \r\n");)
	switch (evt->vr.what) {
	case KVR_Reset:
		gVRReset = true;
		break;
	case KVR_BattChg:
		gVRChargePermitted = evt->vr.value;
		break;
	case KVR_IndData:
		gVRIndicateData = evt->vr.value;
		break;
	}
}

static void
dispatchKBEvent(EMEvent *evt)
{
	TRACE_USB(("KBEvent: ");)
	
	switch (evt->sw.key) {
	case ESWK_KeyAction:
		TRACE_USB(("Key Action");)
		break;
	case ESWK_KeyLeft:
		TRACE_USB(("Key Left  ");)
		break;
	case ESWK_KeyRight:
		TRACE_USB(("Key Right ");)
		break;
	}
	
	switch (evt->sw.event) {
	case ESWE_KeyDown:
		TRACE_USB(("Down  ");)
		break;
	case ESWE_KeyUp:
		TRACE_USB(("Up ");)
		break;
	}
	
	TRACE_USB(("\r\n");)
}

/*******************************************************************************
 * Function:	chargeAllowed
 * Returns:		Boolean
 *
 * Summary:		Call once in a while to check if battery charing is allowed
 * NOTE:		Battery charge is allowed in two cases:
 *				1.	USB is configured(=HOST connected and communicating)
 *					and no R/W operation within the last 8 seconds
 *				2.	USB is NOT configured(Dummy charger) and no USB activity
 *					for 8 s since entering USB mass storage mode
 *******************************************************************************/
static Boolean
chargeAllowed()
{
	Boolean isConf;
	
	isConf = (USBST_CONFIGURED==usb_get_state());

	return gVRChargePermitted && (
		  (isConf  && DIFF(gUSBRWEvtTime, tmrGetMS()) > CHARGE_DELAY) ||
		  (!isConf && DIFF(gUSBStartTime, tmrGetMS()) > CHARGE_DELAY)
		   );
}

/*******************************************************************************
 * Function:	puusbBattChgStatus
 * Summary:		Not used for the moment
 *
 *******************************************************************************/
void
puusbBattChgStatus(chgSt *status)
{
	status->chargingAllowed  = pwrIsBattChargeEnabled(); 
	status->batteryIsCharged = false; 
}

static void
puusbClose()
{
	ledDisplay(kLedEnd);				// All LED off and no more updating of LEDs

	usb_stop();							// Stop USB driver
	vTaskDelay(10);
	
	// Disable USB clock.
	AT91F_PMC_CfgSysClkDisableReg(AT91C_BASE_PMC, AT91C_PMC_UDP);
	
	pwrBattCharge(false);				// Disable battery charger
	hpcReleaseUSB();					// Set USB/Audio switch to Audio mode
}

static void
puusbInit(void *bigBuffer)
{
	TRACE_USB(("Application USB\r\n");)
	
	gDataWritten = false;
	
	gVRReset		  = false;
	gVRIndicateData	  = false;
	gVRChargePermitted= true;
	
	ledDisplay(kLedInit);				// All LED off
	pwrBattCharge(false);				// Disable battery charger
	hpcGoUSB();							// Set USB/Audio switch to USB mode
	vTaskDelay(10);

    mst_init_dataton(bigBuffer,			// Is allocated in main and sharde with audio manager
    				 msCallback);		// Init mass storage handler
	usbVenReq_Init();					// Init Vendor request handler
    usb_init(3);						// Init USB driver, interrupt level = 3
}

void
puusbAppl(void *bigBuffer)
{
	EMEvent evt;
	
	puusbInit(bigBuffer);
	
	while (hpcSense(true)==EHPC_USB && !gVRReset) {		// Main USB loop, 200 Hz
		vTaskDelay(5);
		cpuKickWatchDog();

		mst_process();					// Give time to HCC mass storage handler
		usbVenReq_Maintenace();

		if (EM_GetNextEvent(&evt, 0)) {
			switch (evt.source) {
			case KEVS_Keyboard:	 	 dispatchKBEvent(&evt);
				break;
			case KEVS_VendorRequest: dispatchVREvent(&evt);
				break;
			}
		}
		
		pwrBattCharge(chargeAllowed());
		ledMaint();
	}

	puusbClose();
}






#if 0
static void
displayTime()
{
	LongWord rt = rtcGetRtValue();
	Byte sec, min, hour;
	
	sec = (rt/4)    % 60;
	min = (rt/240)  % 60;
	hour= (rt/14400)% 24;
	
	TRACE_USB(("%02d:%02d:%02d\r", hour, min, sec);)
}

//---------------------------------TYPEDEFS---------------------------------------
typedef enum
{
	KLED_Maintenace,
	KLED_Start,
	KLED_Stop,
	KLED_UsbData,
	_KLED_NumLedState
} LedState;

static void
ledStatus(LedState newState)
{
	static LongWord cnt1 = 0, cnt2 = 0;
	static LedState intState;
	Word i;
	LongWord now;

	// State transision
	switch (newState) {
	case KLED_Maintenace:
		now = tmrGetMS();
		if (DIFF(gUSBRWEvtTime, now) > FLASH_TIME_AFTER_DATA &&
			DIFF(gUSBStartTime, now) > FLASH_TIME_AFTER_START)
			intState = KLED_Maintenace;
		break;
		
	case KLED_Start:
		for (i=0; i<=LEDI_Front; i++)
			ledSetLedLevel(i, LEDD_LEVEL_MIN);
		gUSBRWEvtTime = gUSBStartTime = tmrGetMS();
		intState = KLED_UsbData;
		break;
		
	case KLED_Stop:
		for (i=0; i<=LEDI_Front; i++)
			ledSetLedLevel(i, LEDD_LEVEL_MIN);
		intState = KLED_Stop;						// No more updating of LEDs
		break;
		
	case KLED_UsbData:
		intState = KLED_UsbData;
		break;
	}
	
	// Update LED
	switch (intState) {
	case KLED_Maintenace:
		if (!(cnt1%50)) {							// Charge LED indication
			ledSetLedLevel(LEDI_Front, cnt2%6);		// 0..5
			cnt2++;
		}
		cnt1++;	
		break;
	
	case KLED_UsbData:
		if (!(cnt1%10)) {							// Data LED indication
			if (cnt2%2)
				ledSetLedLevel(LEDI_Front, LEDD_LEVEL_MAX);
			else
				ledSetLedLevel(LEDI_Front, LEDD_LEVEL_MIN);
			
			cnt2++;
		}
		cnt1++;	
		break;
	}
}


#endif


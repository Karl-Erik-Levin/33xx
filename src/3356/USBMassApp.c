/* USBMassApp.c
 **
 ** Application that presents the Pickup to a HOST as a USB mass storgage device.
 **
 ** (C)Copyright Dataton Utvecklings AB 2006-2007, All Rights Reserved.
 **
 ** History:
 ** 20060905		Fred J			Created
 ** 20070528		Fred J	
 */
//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\hwBoard.h"

#include "platform\utilities\byteSwap.h"
#include "Platform\FreeRTOS\FreeRTOS.h"			// FreeRTOS
#include "Platform\FreeRTOS\task.h"
#include "Platform\PowerManager.h"
#include "Platform\util\pubFileUtil.h"
#include "Platform\UserInputManager.h"

#include "drv\CpuDrv.h"							// Drivers
#include "drv\pusettingsdrv.h"
#include "drv\TimerDrv.h"
#include "drv\HPCDrv.h"
#include "drv\PowerDrv.h"

#include "Application\ApplicationEvent.h"
#include "Application\USBMassApp.h"
#include "Application\USBVendorRequestHandler.h"
#include "Application\StorageManager.h"
#include "Application\LEDManager.h"
#include "Application\puAppl.h"

#include "drv/hccusb/usb.h"
#include "Platform/hccusb/simple_scsi.h"
//---------------------------------DEFINES----------------------------------------
#define FLASH_DELAY_AFTER_DATA   (1000 * 12)		// 12 seconds.
#define FLASH_DELAY_AFTER_START  (1000 * 18)		// 18 seconds.
#define CHARGE_DELAY_AFTER_DATA  (FLASH_DELAY_AFTER_DATA + 1000 * 2)		// 14 seconds.
#define FILENAMELEN 			 24
#define LED_MAINT_INTERVAL		 40				// mS

//---------------------------------TYPEDEFS---------------------------------------
typedef enum {
	kLedMaint,
	kLedStart,
	kLedBattInfo
} dispState;

typedef enum {
	EAS_Charged,
	EAS_Data,
	EAS_Charging,
	EAS_Error,
	EAS_ForceUpdate,
	_EAS_NumStates
} EAppState;

//----------------------------GLOBAL VARIABLES------------------------------------
static LongWord gChargeTestStartTI;
static LongWord gLastUSBRWEventTI;
static Boolean gDataWritten = false;
static Boolean gResetReq	= false;

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	need2Update
 * Summary:		Search SDcard for file holding pickup program with different version number
 *******************************************************************************/
static Boolean
need2Update()
{
	Word fileVer;
	char filename[FILENAMELEN];

	if (!IsUpdateAvailable(filename, "PUB"))
		return false;
	
	fileVer = GetFileVersion(filename);
	return fileVer != PUSD_GetAppInfo()->appVersion;
}

/*******************************************************************************
 * Function:	displayStatus
 * Summary:		Set front LED flashing
 *******************************************************************************/
static void 
displayStatus(dispState inState)
{
	static LongWord nextMaint;
	static dispState state;
	static EAppState prevState;
	static Word bLevel, cnt1, dispTime;

	EAppState currState;
	ChargeStatus chgSta;
	
	switch (inState) {
	case kLedMaint:
		if (TM_GT(Timer_GetMS(), nextMaint)) {
			nextMaint += LED_MAINT_INTERVAL;
			
			switch (state) {
			case kLedMaint:
				chgSta = PM_GetChargeStatus();
				
				if ((DIFF(gLastUSBRWEventTI, Timer_GetMS()) <		// Fast led flash if:
					  FLASH_DELAY_AFTER_DATA) || 					// 4 sec after USB data transfer
					 (DIFF(gChargeTestStartTI, Timer_GetMS()) < 	// 9 sec after entering USB-mode
					  FLASH_DELAY_AFTER_START) ||
					 USBVenReq_IsLocked()							// On pickup charger request
					)
				{
					currState = EAS_Data;
				}
				else if (chgSta == KPM_ChgSts_BattFull)
				{
					currState = EAS_Charged;
				}

/*				else if (chgSta == KPM_ChgSts_TempFail)
				{
					currState = EAS_Error;
				}
*/
				else
				{
					currState = EAS_Charging;
				}
				
				if (currState != prevState)
				{
					prevState = currState;
					if (currState == EAS_Error)
					{
						LED_Flash(LEDM_FRONT_LED_IX, true, 5, 5, LEDM_FOREVER);
					}
					else if (currState == EAS_Data)
					{
						LED_Pulse(LEDM_FRONT_LED_IX, false, 0, 0);
						LED_Flash(LEDM_FRONT_LED_IX, true, 1, 1, LEDM_FOREVER);
					}
					else if (currState == EAS_Charging)
					{
						LED_Flash(LEDM_FRONT_LED_IX, false, 0, 0, 0);
						LED_Pulse(LEDM_FRONT_LED_IX, true, 1, LEDM_FOREVER);
					}
					else if (currState == EAS_Charged)
					{
						LED_Flash(LEDM_FRONT_LED_IX, false, 0, 0, 0);
						LED_Pulse(LEDM_FRONT_LED_IX, false, 0, 0);
						LED_SetLevel(LEDM_FRONT_LED_IX, LEDM_LED_LEVEL_MAX);
					}
				}
				break;
			case kLedBattInfo:
				if (cnt1 <= bLevel) {
					LED_SetLevel(cnt1, LEDM_LED_LEVEL_MAX);
				} else if (cnt1 > bLevel + dispTime) {
					LED_AllOff();						// All LED off
					prevState = EAS_ForceUpdate;		// Will force update
					state	  = kLedMaint;
				}
				cnt1++;
				break;
			}
		}
		break;
		
	case kLedStart:
		LED_AllOff();
		nextMaint = Timer_GetMS();
		prevState = EAS_ForceUpdate;	// Will force update
		state	  = kLedMaint;
		break;
		
	case kLedBattInfo:
		LED_AllOff();							// Clear LEDs
		dispTime = 1000 / LED_MAINT_INTERVAL;	// Show status in 1 second
		
		// Display on front LED charge temp fail
		if (PM_GetChargeStatus() == KPM_ChgSts_TempFail) {
			LED_Flash(LEDM_FRONT_LED_IX, true, 3, 1, LEDM_FOREVER);
			dispTime = 15000 / LED_MAINT_INTERVAL;		// Show error in 15 seconds
		}
		
		// Get battery charge level
		bLevel = POWD_GetBatteryLevel();
		if (bLevel > BATTERY_LEVEL_FULL) {
			bLevel = BATTERY_LEVEL_FULL;
		}
		else if (bLevel < BATTERY_LEVEL_LOW) {
			bLevel = BATTERY_LEVEL_LOW;
		}
		
		// Map battery level to 12 RING LEDS
		bLevel = (bLevel - BATTERY_LEVEL_LOW) * (LEDM_NUM_RING_LEDS-1) / (BATTERY_LEVEL_FULL - BATTERY_LEVEL_LOW);
		
		// Prepare for kLedMaint calls
		cnt1 = 0;
		state = kLedBattInfo;
		break;
	}
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

	return USBVenReq_ChargingPermitted() && (
		  (isConf  && DIFF(gLastUSBRWEventTI , Timer_GetMS()) > CHARGE_DELAY_AFTER_DATA) ||
		  (!isConf && DIFF(gChargeTestStartTI, Timer_GetMS()) > CHARGE_DELAY_AFTER_DATA)
		   );
}

/*******************************************************************************
 * Function:	dispatchVenReqEvent
 * Summary:		
 *******************************************************************************/
static void	
dispatchVenReqEvent(
					VendorRequestEvent *ve
){
	switch (ve->what) {
		case EVRE_Reset:	gResetReq = true;
			break;
			
		case EVRE_SetHSID:	break;
	}
}

/*******************************************************************************
 * Function:	dispatchKbEvent
 * Summary:		
 *******************************************************************************/
static void
dispatchKbEvent(UIMKeyboardEvent *kbEv) {
	switch (kbEv->key) {
		case EUIMK_Play:
			if (kbEv->what == EUIMEC_KeySingleClick) {
				displayStatus(kLedBattInfo);
			}
			break;
		default:
			break;
	}
}

/*******************************************************************************
 * Function:	dispatchAppEvent
 * Summary:		
 *******************************************************************************/
static void 
dispatchAppEvent(
				 AppEvent *evb
){
	switch (evb->source) {
		case EAES_VendorRequest:
			dispatchVenReqEvent((VendorRequestEvent *)evb);
			break;
	}
}

/*******************************************************************************
 * Function:	dispatchPMEvent
 * Returns:		void
 *
 * Summary:		Dispatch events from power manager
 *				090202 Kalle Power manager will not send any event
 *							 Application has to poll PM_PowerDownRequested
 *							 and PM_GetBatteryState
 *******************************************************************************/
static void 
dispatchPMEvent(
				PMEvent *pev
){
	switch (pev->what) {
		case KPM_Event_BatFull:
			break;
			
		case KPM_Event_BatNormal:
			break;
			
		case KPM_Event_BatLow:
			break;
			
		case KPM_Event_BatCritical:
			break;
	}
}


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
		gLastUSBRWEventTI = Timer_GetMS();		// Note current time.
		
	    if (caller == STSCSI_WRITE_10) {
			gDataWritten = true;
		}
		break;
	}
}

/*******************************************************************************
 * Function:	USBMassApp_Main
 * Summary:		
 *******************************************************************************/
void
USBMassApp_Main()
{
	void *usbBuffers;
	Byte eventBuf[EVENT_MAX_SIZE];
	EMEvent *evb = (EMEvent *)eventBuf;
		
	Boolean stopUSB = false;
	gDataWritten	= false;
	gResetReq		= false;
	
	TRACE_UMA(("Entering USB mass storage mode\n\r");)

	PM_Charge(false);					// Disable charging
	#if !(defined(HSCONFIG) || defined(BLUPDATE))
	puAppl_SetMode(EAM_USB);
	#endif
						
	STOR_CloseFileSystem();
	
	HPC_GoUSB();						// Set USB/Audio switch to USB mode
	vTaskDelay(10);

	gLastUSBRWEventTI = gChargeTestStartTI = Timer_GetMS();
	displayStatus(kLedStart);			// Uppdate all LEDs

	if ((usbBuffers = pvPortMalloc(mst_req_bufsize_dataton())) == NULL) {
		ASSERT(usbBuffers, ("USBMassApp_Main: malloc usbBuffers"));
		CPUForceWatchdogReset();		// RESTART! Big trouble can't allocate memory for USB buffers
	}
		
    mst_init_dataton(usbBuffers,
    				 msCallback);		// Init mass storage handler
	USBVenReq_Init();					// Init Vendor request handler
    usb_init(3);						// Init USB driver

	while (!stopUSB && !gResetReq)
	{
		CPUWatchDogKick();
		vTaskDelay(2);					// Main loop @ 500 Hz
		
		mst_process();					// Give time to HCC mass storage handler

		// Dispatch event into the correct event handler
		while (EM_GetNextEvent(evb, 0) == pdTRUE) {
			switch (evb->source) {
				case KEVS_PowerManager:	dispatchPMEvent((PMEvent *)evb);
					break;
				case KEVS_Application:	dispatchAppEvent((AppEvent *)evb);
					break;
				case KEVS_Keyboard:		dispatchKbEvent((UIMKeyboardEvent *)evb);
					break;
			}
		}
		
		// Leave USB mode if we cant sense 5V power from host
		if (HPC_Sense(false) != EHPC_USB) {
			stopUSB = true;
		}
		
		// Maintenace call
		displayStatus(kLedMaint);		// Update front LED indication
		PM_Charge(chargeAllowed());		// Enable / disable battery charging?
		USBVenReq_Maintenace();
	}
	
	usb_stop();							// Stop USB driver
	vPortFree(usbBuffers);				// Deallocate buffer memory
	vTaskDelay(10);
	
	// Disable USB clock.
	AT91F_PMC_CfgSysClkDisableReg(AT91C_BASE_PMC, AT91C_PMC_UDP);
	
	PM_Charge(false);					// Disable battery charger
	HPC_ReleaseUSB();					// Set USB/Audio switch to Audio mode

	STOR_OpenFileSystem(2);
	if (gDataWritten) {
		#if !(defined(HSCONFIG) || defined(BLUPDATE))
		TID_SetRebuild(true);			// Tell TID manager it has to rebuild cache file
		#endif
		STOR_ChkDisk();					// Check disk
	}
	
	LED_AllOff();
	if (need2Update()) {				// New firmware?
		CPUForceWatchdogReset();		// Yes, reset and let BL take care of it.
	}
	
	TRACE_UMA(("Exiting USB\n\r");)
}


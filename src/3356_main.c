/* 3356_main.c - Dataton 3356 Pickup main program 
**
** Start system and handle switching between application modes
** 
**
** Created 10-10-30	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
**
**/
//---------------------------------INCLUDES---------------------------------------
#include "SpiritMP3Decoder.h"
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\hwBoard.h"					// Drivers
#include "driver\adcDrv.h"
#include "driver\audDrv.h"
#include "driver\cpuDrv.h"
#include "driver\fmtDrv.h"
#include "driver\hpcDrv.h"
#include "driver\i2cDrv.h"
#include "driver\ledDrv.h"
#include "driver\pwrDrv.h"
#include "driver\rtcDrv.h"
#include "driver\tmrDrv.h"

#include "platform\evtMng.h"				// Managers
#include "platform\pwrMng.h"
#include "platform\setMng.h"
#include "platform\FreeRTOS\FreeRTOS.h"		// Real time OS
#include "platform\FreeRTOS\task.h"
#include "platform\hccfat\api_f.h"
#include "platform\hccusb\usb_mst.h"
#include "platform\utilities\flash.h"

#include "3356\pickupAppl.h"				// Application
#include "3356\puusbAppl.h"

//---------------------------------DEFINES----------------------------------------
#define SOFTWARE_VERSION	0x000902		// 0.9B2

//----------------------------GLOBAL VARIABLES------------------------------------

//--------------------------------PROTOTYPS---------------------------------------
void vApplicationIdleHook(void);

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	drvInit
 * Summary:		Initialize the basic driver set
 *******************************************************************************/
static void
drvInit()
{
	// Feed clock to PIO
    AT91F_PMC_EnablePeriphClock(AT91C_BASE_PMC, (unsigned int)1 << AT91C_ID_PIOA);

    // Feed clock to system devices.
    AT91F_PMC_EnablePeriphClock(AT91C_BASE_PMC, (unsigned int)1 << AT91C_ID_SYS);

	// Prepare AIC for new interrupt 
    AT91C_BASE_AIC->AIC_EOICR = 0;
	
	// Enable brownout detector
	AT91F_Flash_Enable_Brownout();

#if (FREERTOS_TIMER == PIT)
	tmrInit();					// uS and mS timer.
#else			
	// If (FREERTOS_TIMER == TC1) will vTaskStartScheduler() call
	// tmrInit() and tmrDrv will call FreeRTOS on timer interrupt 
	// for task scheduling
#endif

	rtcInit();					// Real time clock
	adcInit();					// A/D converter
	audSpeakerAmp(false);		// Disable speaker
	hpcInit();					// Headphone conector 
	i2cInit();					// I2C driver
	ledInit();					// LED driver
	pwrInit();					// Power driver
}

/*******************************************************************************
 * Function:	vApplicationIdleHook
 * Summary:		This function will be called from FreeRTOS IDLE task
 *				MUST NOT, UNDER ANY CIRCUMSTANCES, CALL A FUNCTION THAT MIGHT BLOCK
 *******************************************************************************/
void
vApplicationIdleHook(void)
{
	// Halt CPU to save power, first interrupt will start CPU
	AT91F_PMC_CfgSysClkDisableReg(AT91C_BASE_PMC, AT91C_PMC_PCK);
}

/*******************************************************************************
 * Function:	mainTask
 * Summary:		Main program. Will initialize system and handle switching
 *				between the two application mode
 *******************************************************************************/
static void
mainTask(void *parameters)
{
	int temp;
	void *bigBuffer;

	cpuInitWatchDog();			// Start watchdog. Period 3,0 seconds
	drvInit();					// Init drivers
	
	// Need to update firmware version ?
	prdInfoInit();				
	if (prdInfoGet()->swVer != SOFTWARE_VERSION) {
		prdInfoGet()->swVer  = SOFTWARE_VERSION;
		prdInfoStore();
	}
	firmwareInfo(false);		// Firmware info to debug port
	
	EM_Init();					// Init event manager
	pwrMngInit();				// Init power manager
	
	temp = f_init();			// Prepare filesystem
	ASSERT(temp==F_NO_ERROR, ("mainTask: Filesystem failed to initialize: %d",temp));
	
	// Allocate a large buffer that is shared usb-stack and mp3 decoder
	bigBuffer = pvPortMalloc(MAX(mst_req_bufsize_dataton(), sizeof(TSpiritMP3Decoder)));
	ASSERT(bigBuffer, ("mainTask: Can't allocate big buffer"));
	
	while (1)					// Stay here forever
	{
		cpuKickWatchDog();
		
		switch (hpcSense()) {
		case EHPC_USB:
			puusbAppl(bigBuffer);
			break;
		default:
			pickupAppl(bigBuffer);
			break;
		}
	}
}

/*******************************************************************************
 * Function:	main
 * Summary:		We will start here ...
 *******************************************************************************/
int
main(void)
{
	xTaskCreate(mainTask,
			    "MAIN",
			    400,
			    NULL,
			    (tskIDLE_PRIORITY + 1),
			    (xTaskHandle *) NULL );
	
    vTaskStartScheduler();		// Start FreeRTOS and never return
    return 0;
}


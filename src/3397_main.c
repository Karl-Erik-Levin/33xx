/* 3397_main.c - Dataton 3397 Transponder main program 
**
** Start system and handle switching between application modes
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
#include "driver\adcDrv.h"
#include "driver\cpuDrv.h"
#include "driver\fmtDrv.h"
#include "driver\hpcDrv.h"
#include "driver\i2cDrv.h"
#include "driver\ledDrv.h"
#include "driver\pwrDrv.h"
#include "driver\rtcDrv.h"
#include "driver\tmrDrv.h"

#include "3397\micAppl.h"				// Application
#include "3397\trspAppl.h"	
#include "3397\usbAppl.h"

#include "platform\setMng.h"
#include "platform\FreeRTOS\FreeRTOS.h"		// OS
#include "platform\FreeRTOS\task.h"

#include "platform\evtMng.h"
#include "platform\utilities\flash.h"

//---------------------------------DEFINES----------------------------------------
#define SOFTWARE_VERSION	0x000101		// 0.1B01

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
	hpcInit();					// Headphone conector 
	fmtInit();					// FM Transciever driver, select I2C mode
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
 * Function:	displayInfo
 * Summary:		Send application info to debug port
 *******************************************************************************/
static void
displayInfo(void)
{
#ifdef DEBUG
	prdInfo *ai = prdInfoGet();
	
	TracePrintf("\r\n\r\n\r\n");
	TracePrintf("*----------------------------*\r\n");
	TracePrintf("* START TRANSPONDER FIRMWARE *\r\n");
	TracePrintf("*                            *\r\n");
	TracePrintf("* Hardware ver: %d.%d.%d        *\r\n", (ai->hwVer>>16)%256, (ai->hwVer>>8)%256, ai->hwVer%256);
	TracePrintf("* Software ver: %d.%dB%d        *\r\n", (ai->swVer>>16)%256, (ai->swVer>>8)%256, ai->swVer%256);
	TracePrintf("* SerialNumber: %5d        *\r\n", ai->serialNumber);
	TracePrintf("* Transponder : %04d         *\r\n", ai->irID);
	TracePrintf("*                            *\r\n");
	TracePrintf("*----------------------------*\r\n");
#endif	
}

/*******************************************************************************
 * Function:	mainTask
 * Summary:		Main program. Will initialize system and handle switching
 *				between the three application mode
 *******************************************************************************/
static void
mainTask(void *parameters)
{
	cpuInitWatchDog();			// Start watchdog. Period 3,0 seconds
	drvInit();					// Init drivers
	
	prdInfoInit();				// Get transponder ID etc..
	if (prdInfoGet()->swVer != SOFTWARE_VERSION) {
		prdInfoGet()->swVer = SOFTWARE_VERSION;
		prdInfoStore();
	}
	displayInfo();
	
	EM_Init();					// Init event manager
	
	while (1)					// Stay here forever
	{
		cpuKickWatchDog();
		
		switch (hpcSense()) {
		case EHPC_USB:
			usbAppl();
			break;
		case EHPC_MIC:
			micAppl();
			break;
		default:
			trspAppl();
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




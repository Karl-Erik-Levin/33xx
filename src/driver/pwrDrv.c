/* pwrDrv.c - Dataton 33xx
**
**	Power Driver
** 		
**	Handle 1) power and battery charger circuit LTC3567
**		   2) User switches
**
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/
//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\hwBoard.h"
#include "driver\i2cdrv.h"
#include "driver\rtcDrv.h"
#include "driver\tmrDrv.h"

#include "platform\evtMng.h"		// pwrDrv.h

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"
#include "platform\FreeRTOS\semphr.h"

//---------------------------------DEFINES----------------------------------------
#define PIO_IRQ_PRIO 		4

//----------------------------GLOBAL VARIABLES------------------------------------
static Byte		g3567State[] = {0xFF, 0x00};
static LongWord pioInput;

static xSemaphoreHandle pwrSem, pwrDownSem;

//--------------------------------PROTOTYPES---------------------------------------
void pwrDrvISR();
void pwrDrvISREntry();

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	pwrDrvISR
 * 
 * Summary:		Handle interrupt when user switch has change
 *******************************************************************************/
void
pwrDrvISR()
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	LongWord isrStatus = AT91C_BASE_PIOA->PIO_ISR;
	
	pioInput = AT91F_PIO_GetInput(AT91C_BASE_PIOA);
	
	xSemaphoreGiveFromISR(pwrSem, &xHigherPriorityTaskWoken);
	
	/* If a task was woken by call from ISR then we may need
	   to switch to another task. */
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);

	/* End the interrupt in the AIC. */
	AT91C_BASE_AIC->AIC_EOICR = 0;
}

/*******************************************************************************
 * Function:	pwrDownTask
 * 
 * Summary:		FreeRTOS Task. 
 *******************************************************************************/
static void
pwrDownTask(void *params)
{
	int cnt = 0;
	
	xSemaphoreTake(pwrDownSem, 0);		// Compensate for stupid macro vSemaphoreCreateBinary()
										// that will perform a xSemaphoreGive()
	
	while (1) {
		if (xSemaphoreTake(pwrDownSem, portMAX_DELAY) == pdPASS) {
			if (cnt++ & 1)
				TRACE_PWR((" ");)
			TRACE_PWR(("pwr dwn\r\n");)
			
			LTC3567BurstMode(true);		// Set 3,3V reg in burst mode to reduce current
			tmrWaitMS(4);
			
			tmrSleep();					// Timer and OS sleep
			rtcSleep(0);				// Realtime clock interrupt. No wake interrupt
			
			// Halt CPU, will start on any interrupt
			AT91F_PMC_CfgSysClkDisableReg(AT91C_BASE_PMC, AT91C_PMC_PCK | AT91C_PMC_UDP);
			
			rtcWake();
			tmrWake();
			
			LTC3567BurstMode(false);	// Set 3,3V reg in normal mode
		}
	}
}

/*******************************************************************************
 * Function:	pwrTask
 * 
 * Summary:		FreeRTOS Task. Take interrupt from user switches and post
 *				event to application
 *******************************************************************************/
static void
pwrTask(void *params)
{
	static LongWord prevInput;
	static EMEvent	evt;
	LongWord chgInput;
	
	while (1) {
		if (xSemaphoreTake(pwrSem, portMAX_DELAY) == pdPASS) {
			chgInput = (prevInput ^ pioInput);
//			TRACE_PWR(("pwrTask: time=%d input=%x chg=%x\r\n", tmrGetMS(), pioInput, chgInput);)
			evt.source = KEVS_Keyboard;
			
			if (chgInput & KB_PLAY_PIN) {
				evt.sw.key = ESWK_KeyAction;
				
				if (pioInput & KB_PLAY_PIN)
					evt.sw.event = ESWE_KeyUp;
				else
					evt.sw.event = ESWE_KeyDown;
				
				EM_PostEvent(&evt, 0);
			}
			
			if (chgInput & KB_LEFT_PIN) {
				evt.sw.key = ESWK_KeyLeft;
				
				if (pioInput & KB_LEFT_PIN)
					evt.sw.event = ESWE_KeyUp;
				else
					evt.sw.event = ESWE_KeyDown;
				
				EM_PostEvent(&evt, 0);
			}
			
			if (chgInput & KB_RIGHT_PIN) {
				evt.sw.key = ESWK_KeyRight;
				
				if (pioInput & KB_RIGHT_PIN)
					evt.sw.event = ESWE_KeyUp;
				else
					evt.sw.event = ESWE_KeyDown;
				
				EM_PostEvent(&evt, 0);
			}
			prevInput = pioInput;
		}
	}
}

/*******************************************************************************
 * Function:	pwrInit
 * 
 * Summary:		Enable 3,3V regulator to take power from battery
 *				Configur PIO for two user input switches. Changes in these 
 *				switces generate interrupt. Interrupt routine inform pwrTask
 *				about changes via semaphore pwrSem. pwrTask send event to 
 *				application to inform about switch has been pressed by user
 *******************************************************************************/
void
pwrInit(void)
{
  #if (HARDWARE == 3356)
  	// Pickup hardware version < 2.0
	AT91F_PIO_CfgOutput(AT91C_BASE_PIOA, BATT_CHARGE_ENABLE_PIN);	// Output
	AT91C_BASE_PIOA->PIO_PPUDR = BATT_CHARGE_ENABLE_PIN;			// Disable pullup
	pwrBattChargeEnable(false);
  #endif
  	
	LTC3567BuckBoostEnable(true);

	// Configure key switches pins as pullup input pins
	AT91F_PIO_CfgInput(AT91C_BASE_PIOA, KB_PINS_MASK);
	AT91F_PIO_CfgInputFilter(AT91C_BASE_PIOA, KB_PINS_MASK);
	
	AT91C_BASE_PIOA->PIO_PPUER = KB_PINS_MASK |				// Enable pullup
								 POWD_BATT_CHARGE_IND;
	AT91C_BASE_PIOA->PIO_PPUDR = DBGU_RX|DBGU_TX;			// Disable pullup for trace port here
															// in case it's not init
	
	vSemaphoreCreateBinary(pwrSem);			// Create semaphore
	xTaskCreate(pwrTask, 					// Create task
                "PWRT", 
                100, 
                NULL,
				(tskIDLE_PRIORITY + 2),
                NULL);

#if 0
	vSemaphoreCreateBinary(pwrDownSem);		// Create semaphore
	xTaskCreate(pwrDownTask, 				// Create task
                "PWDT", 
                100, 
                NULL,
				(tskIDLE_PRIORITY + 5),		// Highest priority in system
                NULL);
#endif

	// Enable interrupt from key switches
	AT91F_PIO_InterruptEnable(AT91C_BASE_PIOA, KB_PINS_MASK);
	AT91F_AIC_ConfigureIt(
				AT91C_BASE_AIC, 
				AT91C_ID_PIOA, 
				PIO_IRQ_PRIO, 
				AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE, 
				pwrDrvISREntry); 
	AT91C_BASE_AIC->AIC_ICCR = 1 << AT91C_ID_PIOA;
	AT91F_AIC_EnableIt(AT91C_BASE_AIC, AT91C_ID_PIOA);
}

/*******************************************************************************
 * Function:	requestPowerDown
 * Summary:		Called from application when it need to power down
 *******************************************************************************/
void
pwrDownRequest(void)
{
#if 0
	xSemaphoreGive(pwrDownSem);
#endif
}

/*******************************************************************************
 * Function:	pwrBattChargeEnable
 * 
 * Summary:		Enable/Disable charging of battery to save USB host current
 *******************************************************************************/
void 
pwrBattChargeEnable(Boolean enable)
{
  #if (HARDWARE == 3356)
	if (enable) {
		AT91F_PIO_ClearOutput(AT91C_BASE_PIOA, BATT_CHARGE_ENABLE_PIN);
	}
	else {
		AT91F_PIO_SetOutput(AT91C_BASE_PIOA, BATT_CHARGE_ENABLE_PIN);
	}
  #endif
}

/*******************************************************************************
 * Function:	LTC3567BuckBoostEnable
 * 
 * Summary:		Enable/Disable 3,3V regulator to take power from battery
 *******************************************************************************/
void
LTC3567BuckBoostEnable(Boolean enable)
{
	if (enable)
		g3567State[1] |= 0x04;
	else
		g3567State[1] &= ~0x04;
		
	i2cWrite(EI2CD_3567, 0, g3567State, 2);
}

/*******************************************************************************
 * Function:	LTC3567BurstMode
 * 
 * Summary:		Enable 3,3V regulator to take power from battery
 *******************************************************************************/
void
LTC3567BurstMode(Boolean burstOn)
{
	if (burstOn)
		g3567State[1] |= 0x40;
	else
		g3567State[1] &= ~0x40;
		
	i2cWrite(EI2CD_3567, 0, g3567State, 2);
}

/*******************************************************************************
 * Function:	LTC3567CurrentMode
 * 
 * Summary:		Enable 3,3V regulator to take power from battery
 *******************************************************************************/
void
LTC3567CurrentMode(USBCurrentMode mode)
{
	g3567State[1] &= ~0x03;
	g3567State[1] |= mode;
	i2cWrite(EI2CD_3567, 0, g3567State, 2);
}

/*******************************************************************************
 * Function:	LTC3567ServoVoltage
 * 
 * Summary:		Enable 3,3V regulator to take power from battery
 *******************************************************************************/
void
LTC3567ServoVoltage(Byte mode)
{
	g3567State[0] &= ~0x0F;
	g3567State[0] |= mode;
	i2cWrite(EI2CD_3567, 0, g3567State, 2);
}



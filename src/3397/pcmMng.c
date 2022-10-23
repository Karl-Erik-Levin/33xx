/* audMng.c - Dataton 3397 Transponder 
**
** Audio manager
**
** This module is for testing  plaback audio in Pickup speaker via MAX9850 D/A converter.
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
*/
//---------------------------------INCLUDES---------------------------------------
#include <math.h>
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\audDrv.h"
#include "driver\tmrDrv.h"

#include "platform\audMng.h"

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"
#include "platform\FreeRTOS\queue.h"
#include "platform\FreeRTOS\semphr.h"

//---------------------------------DEFINES----------------------------------------
#define PB_CMD_INIT_MP3					1			// Internal commands.
#define PB_CMD_INIT_WAV					2
#define PB_CMD_PLAY						3
#define PB_CMD_PAUSE					4
#define PB_CMD_STOP						5
#define PB_CMD_SKIP_FWD					6
#define PB_CMD_NEXT						7
#define PN_CMD_SET_PLAYBACK_SPEED		8
#define PB_CMD_INIT_FILE				9
#define PB_CMD_SET_TIME					10
#define PB_CMD_SET_SLEEP				11
#define PB_CMD_SET_DELAY				12
#define PB_CMD_SET_LOOP					13
#define PB_CMD_CLOSE_PB					14

#define FRAME_SIZE_SAMPLES				1152		// Size of the output buffer in samples
													// 1 samples = 32 bits - 16 bits for left channel + 16 bits for right channel
													// One buffer will play approx 26 ms @44,1 kHz
														
#define PB_TASK_STACK_SIZE				500
#define PB_TASK_NAME					"PBTK"
#define PB_QUEUE_SIZE					5

//---------------------------------TYPEDEFS---------------------------------------
typedef struct _PBCommand
{
	Byte 			cmd;			// What command.
	Word			sr;				// sampleRate
	LongWord 		when;			// When where command initiated.
} PBCommand;

typedef enum
{
	PBS_End,
	PBS_Play,
	PBS_CueUp,
	PBS_Delay,
	_PBS_NumStates
} PBState;

typedef struct _PlayBackInfo
{
	LongWord 		pbStart;			// Time in mS at playback start.
	Word 			pbSampleRate;		// Current sample rate
	LongWord		frame;				// Counter of pcmBufs
	short *			pcmBuf[2];			// Buffers to hold audio sample data
	PBState 		iState;				// Internal state.
} PlayBackInfo;

//------------------------------GLOBAL VARIABLES----------------------------------
static xQueueHandle pbEventQueue;	
static xSemaphoreHandle audSem;
static xTaskHandle gPBTaskHandle;

static PlayBackInfo pi;

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	createSound
 * Returns:		void
 * 
 * Summary:		
 *******************************************************************************/
 static void
 createSound()
 {
 	int		i, cnt;
 	short	l;
 	
 	for (i=0, cnt=0; i<FRAME_SIZE_SAMPLES*2; i+=2) {
 		l = 20000 * sin(6.28 * cnt / 48) + 10000 * sin(6.28 * cnt / 24);
// 		r = 10000 * sin(6.28 * cnt / 24);
 		pi.pcmBuf[0][i]   = l;
 		pi.pcmBuf[0][i+1] = l;
 		cnt++;
 		
 		pi.pcmBuf[1][i]   = l;
 		pi.pcmBuf[1][i+1] = l;
 	}
 }
 
/*******************************************************************************
 * Function:	playback
 * Returns:		void
 * 
 * Summary:		
 *******************************************************************************/
 static void
 playback()
 {
 	if (audPlayBuffer(pi.pcmBuf[pi.frame & 1], FRAME_SIZE_SAMPLES*2)) 
 	{
 		pi.frame++;
 		TRACE_AUD(("audMng: ms=%d frame=%d\r\n", tmrGetMS(), pi.frame);)
 	}
 }
 
/*******************************************************************************
 * Function:	pbTask
 * Returns:		void
 * 
 * Summary:		Playback manager "main"-loop
 *******************************************************************************/
static void
pbTask(void *parameters)
{
	while (1) {
		PBCommand cmd;
		
		// Handle new commands and set pbTask blocktime
		if (xQueueReceive(pbEventQueue, &cmd, 4) == pdTRUE) {
			if (cmd.cmd == PB_CMD_PLAY) {
				pi.pbStart = tmrGetMS();
				pi.pbSampleRate = cmd.sr;
				pi.frame = 0;
				pi.iState = PBS_Play;
				
				createSound();
				
				audSetPlaybackRate(pi.pbSampleRate);
				
				TRACE_AUD(("audMng: Play cmd after %d ms\r\n", pi.pbStart-cmd.when);)
			}
			else if (cmd.cmd == PB_CMD_STOP) {			
				pi.iState = PBS_End;
			}
			else if (cmd.cmd == PB_CMD_CLOSE_PB) {			
				pi.iState = PBS_End;
			}
			
			xSemaphoreGive(audSem);				// Command is handled
		}
		
		// Handle playback of sound
		if (pi.iState == PBS_Play) {
			playback();
		}
	}
}

/*******************************************************************************
 * Function:	pbExecuteAndWait
 * Returns:		Boolean
 * 
 * Summary:		
 *******************************************************************************/
static Boolean 
pbExecuteAndWait(
	PBCommand *cmd, 
	Word sendWaitMS, 
	Word ackWaitMS
){
	xQueueSend(pbEventQueue, cmd, sendWaitMS);
	
	if (xSemaphoreTake(audSem, ackWaitMS) == pdTRUE) {
		return true;
	}
	return false;
}

/*******************************************************************************
 * Function:	PB_Play
 * Returns:		void
 * 
 * Summary:		
 *******************************************************************************/
void
PB_Play(Word sampleRate)
{
	PBCommand cmd;
	
	cmd.cmd = PB_CMD_PLAY;
	
	if (sampleRate < AD_PLAYBACK_RATE_MIN) sampleRate = AD_PLAYBACK_RATE_MIN;
	if (sampleRate > AD_PLAYBACK_RATE_MAX) sampleRate = AD_PLAYBACK_RATE_MAX;
	cmd.sr = sampleRate;
	
	cmd.when = tmrGetMS();
	
	pbExecuteAndWait(&cmd, 10, 50);	// Block until command is handled
	TRACE_AUD(("audMng: Play cmd compleated after %d ms\r\n", tmrGetMS()-cmd.when);)
}

/*******************************************************************************
 * Function:	PB_Stop
 * Returns:		void
 * 
 * Summary:		
 *******************************************************************************/
void
PB_Stop()
{
	PBCommand cmd;
	
	cmd.cmd = PB_CMD_STOP;

	pbExecuteAndWait(&cmd, 10, 50);	// Block until command is handled
}

/*******************************************************************************
 * Function:	PB_SetVolume
 * Returns:		void
 * 
 * Summary:		
 *******************************************************************************/
void 
PB_SetVolume(
	Word newVolume
){
	if (newVolume > PB_VOLUME_MAX) {
		newVolume = PB_VOLUME_MAX;
	}
	
	audSetVolume((newVolume * AD_VOLUME_MAX) / PB_VOLUME_MAX);	
}

/*******************************************************************************
 * Function:	PB_Init
 * Returns:		void
 * 
 * Summary:		Call once to initilize the playback unit
 *******************************************************************************/
void
PB_Init()
{
	pbEventQueue = xQueueCreate(PB_QUEUE_SIZE, sizeof(PBCommand));   		// Create PB Work Queue
	
	pi.pcmBuf[0] = pvPortMalloc(FRAME_SIZE_SAMPLES * 2 * sizeof(short));	// Allocate buffers
	pi.pcmBuf[1] = pvPortMalloc(FRAME_SIZE_SAMPLES * 2 * sizeof(short));

	audSem = xSemaphoreCreateMutex();										// Create semaphore
	
	xTaskCreate(						// Start playback task
				pbTask,
				PB_TASK_NAME,
				PB_TASK_STACK_SIZE,
				NULL,
				(tskIDLE_PRIORITY + 4),
				&gPBTaskHandle);
}

/*******************************************************************************
 * Function:	PB_Close
 * Returns:		void
 * 
 * Summary:		
 *******************************************************************************/
void
PB_Close()
{
	PBCommand cmd;								
	
	// Send CLOSE-CMD to PB.
	cmd.cmd = PB_CMD_CLOSE_PB;
	pbExecuteAndWait(&cmd, 10, 50);		// Block until command is handled

	vTaskDelete(gPBTaskHandle);
	vQueueDelete(pbEventQueue);
	vQueueDelete(audSem);

	if (pi.pcmBuf[0])
		vPortFree(pi.pcmBuf[0]);
	
	if (pi.pcmBuf[1])
		vPortFree(pi.pcmBuf[1]);
}

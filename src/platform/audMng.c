/* audMng.c - Dataton 3356 Pickup
**
** Audio manager
**
** This module is for playback audio in headphone and speaker via MAX9850 D/A converter.
**
**
** Created 10-10-30	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
*/
//---------------------------------INCLUDES---------------------------------------
#include <math.h>

#include "SpiritMP3Decoder.h"
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\adcDrv.h"
#include "driver\audDrv.h"
#include "driver\hpcDrv.h"
#include "driver\tmrDrv.h"

#include "platform\hccfat\api_f.h"
#include "platform\audMng.h"
#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"
#include "platform\FreeRTOS\queue.h"
#include "platform\FreeRTOS\semphr.h"
#include "platform\utilities\mp3Util.h"
#include "3356\WavWriter.h"

//---------------------------------DEFINES----------------------------------------
#define PB_CMD_INIT_MP3					1			// Internal commands.
#define PB_CMD_INIT_WAV					2
#define PB_CMD_INIT_SIN					3
#define PB_CMD_PLAY						4
#define PB_CMD_PAUSE					5
#define PB_CMD_STOP						6
#define PB_CMD_CLOSE_PB					7

#define FRAME_SIZE_SAMPLES				1152		// Size of the output buffer in samples
													// 1 samples = 32 bits - 16 bits for left channel + 16 bits for right channel
													// One buffer will play approx 26 ms @44,1 kHz
														
#define PB_TASK_STACK_SIZE				500
#define PB_TASK_NAME					"PBTK"
#define PB_QUEUE_SIZE					5

#define MDCT_LOW						20			// Low frequency range border
#define MDCT_MID						80			// Mid frequency range border
#define MDCT_COEF_COUNT					576			// number of MDCT coefficients

#define VOLSAMP_INTERVAL				200			// Sample every 200 ms

//---------------------------------TYPEDEFS---------------------------------------
typedef struct _PBCommand
{
	Byte 			cmd;			// What command.
	Word			sr;				// sampleRate
	char			*filename;		// OR open file with name
	LongWord		startCluster;	// OR open file startCluster and fileSize
	LongWord		fileSize;
	LongWord 		when;			// When where command initiated.
} PBCommand;

typedef enum
{
	PBS_CueUp,
	PBS_Play,
	PBS_Pause,
	PBS_Stop,
	_PBS_NumStates
} PBState;

typedef enum
{
	PBT_Mp3,
	PBT_Wav,
	PBT_Sin,
	_PBT_NumStates
} PBType;

typedef struct _PlayBackInfo
{
	PBState 		iState;				// Internal state.
	PBType	 		pbType;				// Playback type.
	LongWord 		pbStart;			// Time in mS at playback start.
	LongWord		volSamp;			// Time when volyme pot is sampled
	Word 			pbSampleRate;		// Current sample rate
	Word			pbBytePerMs;		// Data rate, bytes per ms
	Word			pbNumSamp;
	F_FILE *		file;				// Audio file
	LongWord		frame;				// Counter of pcmBufs
	short *			pcmBuf[2];			// Buffers to hold audio sample data
} PlayBackInfo;

//------------------------------GLOBAL VARIABLES----------------------------------
static xQueueHandle pbEventQueue;	
static xSemaphoreHandle audSem;
static xTaskHandle gPBTaskHandle;

static short sampBuf0[FRAME_SIZE_SAMPLES * 2];
static short sampBuf1[FRAME_SIZE_SAMPLES * 2];

static PlayBackInfo pi;

static TSpiritMP3Decoder *mp3_Decoder;	// Decoder object
static TSpiritMP3Info     mp3_Info;		// MP3 audio information

//--------------------------------FUNCTIONS---------------------------------------
Word getEQ(void);
Word getEQ(void)
{
	return 0;
}

#define PI 3.1415926535
/*******************************************************************************
 * Function:	createSinSound
 * 
 * Summary:		Just for testing
 *******************************************************************************/
 static short
 createSinSound()
 {
 	int		idx, cnt;
 	short	l, r;
 	
 	for (cnt=0; cnt<FRAME_SIZE_SAMPLES; cnt++) {
 		l = 20000 * sin(2*PI * cnt / 96);
 		r = 20000 * sin(2*PI * cnt / 24);
 		idx = cnt*2;
 		
 		pi.pcmBuf[0][idx]   = l;
 		pi.pcmBuf[0][idx+1] = r;
 		
 		pi.pcmBuf[1][idx]   = l;
 		pi.pcmBuf[1][idx+1] = r;
 	}
 	return FRAME_SIZE_SAMPLES;
 }
 
/*******************************************************************************
 * Function:	audioVolume
 * Summary:		Sample volume pot and set audio level
 *******************************************************************************/
static void
audioVolume(void)
{
	Word pot = adcGetLevel(EADC_Volume);
	LongWord vol = pot * AD_VOLUME_MAX / POTD_LEVEL_MAX;
	
	audSetVolume(vol);
}

/*******************************************************************************
 * Function:	spiritEQsettingCallback
 * Summary:		Post process MP3-data
 *******************************************************************************/
static void
spiritEQsettingCallback(
    int *pMDCTcoeff,		// [IN/OUT] MDCT coefficients (576 samples)
    int isShort,			// flag: 1 if short frame
    int ch,					// channel number (0 or 1).
    void *token)			// Optional parameter for callback function
{
    int i;
    
    switch (getEQ()) {
	case 1:
		for (i = 0; i < MDCT_LOW; i++) {						// Apply +6 dB boost for low-frequency range
			pMDCTcoeff[i] = pMDCTcoeff[i] - pMDCTcoeff[i] / 2;
		}
		break;
		
	case 2:											
		for (i = 0; i < MDCT_LOW; i++) {						// Apply +6 dB boost for low-frequency range
			pMDCTcoeff[i] = pMDCTcoeff[i] - pMDCTcoeff[i] / 2;
		}
		for (i = MDCT_MID; i < MDCT_COEF_COUNT; i++) {				// Apply +6 dB boost for high-frequency range
			pMDCTcoeff[i] <<= 1;
		}
		break;
		
	case 3:
		for (i = MDCT_MID; i < MDCT_COEF_COUNT; i++) {				// Apply +6 dB boost for high-frequency range
			pMDCTcoeff[i] <<= 1;
		}
		break;
		
	case 4:
		for (i = MDCT_LOW; i < MDCT_MID; i++) {						// Apply +6 dB boost for mid-frequency range
			pMDCTcoeff[i] <<= 1;
		}
		break;
		
	default:
		break;
    }    
}

/*******************************************************************************
 * Function:	spiritMP3ReadCallback 
 * Summary:		Callback called from Spirit MP3 decoder to retrive some 
 *				data from the MP3 file
 *******************************************************************************/ 
static unsigned int
spiritMP3ReadCallback(
    void *pMP3CompressedData,        	// Buffer to hold MP3 data
    unsigned int nMP3DataSizeInChars, 	// Number of bytes of MP3 data to get.
    void *token)                      	// Application-supplied parameter (File pointer).
{
	long bytesReadFromFile;
	
	bytesReadFromFile = f_read(pMP3CompressedData, 1, nMP3DataSizeInChars, token);
 	TRACE_AUD(("ReqRead=%5d FileRead=%5d", nMP3DataSizeInChars, bytesReadFromFile);)
	
	if (bytesReadFromFile < 0)
		bytesReadFromFile = 0;
	
	if (bytesReadFromFile < nMP3DataSizeInChars) {
		pi.iState = PBS_Stop;
	}
	
	return bytesReadFromFile;
}

/*******************************************************************************
 * Function:	readWavFile
 * 
 * Summary:		
 *******************************************************************************/
static unsigned int
readWavFile(F_FILE *fp, short *pcmBuf, Word numSamp)
{
	unsigned int samp = f_read(pcmBuf, sizeof(short)*2, numSamp, fp);
	
	if (samp <numSamp) {
		pi.iState = PBS_Stop;
		WAV_readerClose(fp);
	}
	
	return samp;
}

/*******************************************************************************
 * Function:	playback
 * 
 * Summary:		
 *******************************************************************************/
 static void
 playback()
 {
	static LongWord lastTime;
	
 	if (audPlayBuffer(pi.pcmBuf[pi.frame & 1], pi.pbNumSamp)) 
 	{
 		LongWord now = tmrGetMS();
 		
 		pi.frame++;
 		
 		// Get more audio sample
 		switch (pi.pbType) {
 		case PBT_Mp3:
			pi.pbNumSamp = SpiritMP3Decode(mp3_Decoder, pi.pcmBuf[pi.frame & 1], FRAME_SIZE_SAMPLES, &mp3_Info);
 			break;
 			 
 		case PBT_Wav:
			pi.pbNumSamp = readWavFile(pi.file, pi.pcmBuf[pi.frame & 1], FRAME_SIZE_SAMPLES);
 			break;
 		}
 		
 		TRACE_AUD(("DcTm=%6d \r\n", now-lastTime);)
 		lastTime = now;
 	}
 	
 	if (DIFF(pi.volSamp, tmrGetMS()) > VOLSAMP_INTERVAL) {
		audioVolume();
	 	audSpeakerAmp(hpcSense() == EHPC_VOID);		// Enable/Disable speaker
	 	pi.volSamp = tmrGetMS();
 	}
 }
 
/*******************************************************************************
 * Function:	prepareStartMp3File
 * 
 * Summary:		Position file to begining of MP3 data and then decode first frame
 *******************************************************************************/
static Boolean
prepareStartMp3File(LongWord startCluster, LongWord fileSize)
{
	Boolean fileFound = false;
	
	// Close old file if it's still open
	if(pi.file)	{
		f_close(pi.file);
	}
	
	// Open the new file
	f_chdir("A:/");
	pi.file = f_openopt(startCluster, fileSize);
	ASSERT(pi.file, ("prepareStartMp3File: Couldn't open file"));
	
	if (pi.file) {
		if (MP3_FindFirstFrame(pi.file) > 0) {
			pi.frame  = 0;
			pi.pbType = PBT_Mp3;
			
			SpiritMP3DecoderInit(mp3_Decoder, spiritMP3ReadCallback, pi.file, spiritEQsettingCallback);
			pi.pbNumSamp = SpiritMP3Decode(mp3_Decoder, pi.pcmBuf[0], FRAME_SIZE_SAMPLES, &mp3_Info);

			if (pi.pbNumSamp > 0) {
				pi.pbSampleRate = mp3_Info.nSampleRateHz;
				pi.pbBytePerMs = MP3_BytesPerMS(mp3_Info.nBitrateKbps);

				audSetPlaybackRate(mp3_Info.nSampleRateHz);
				fileFound = true;
				pi.pbStart  = tmrGetMS();
			}
		}
	}
	return fileFound;
}

/*******************************************************************************
 * Function:	prepareStartWavFile
 * 
 * Summary:		Position file to begining of MP3 data and then decode first frame
 *******************************************************************************/
static Boolean
prepareStartWavFile(char *wavFile)
{
	Boolean fileFound = false;
	wavProp prop;

	f_chdir("A:/");
	pi.file =  WAV_readerOpen(wavFile, &prop);

	if (pi.file != NULL) {
		pi.frame = 0;
		pi.pbType   = PBT_Wav;
		pi.pbStart  = tmrGetMS();
		pi.pbSampleRate = prop.nSampleRateHz;
		pi.pbBytePerMs  = 0;
		pi.pbNumSamp = readWavFile(pi.file, pi.pcmBuf[0], FRAME_SIZE_SAMPLES);

		audSetPlaybackRate(prop.nSampleRateHz);
		fileFound = true;
	}
	
	return fileFound;
}

/*******************************************************************************
 * Function:	pbTask
 * 
 * Summary:		Audio manager "main"-loop
 *******************************************************************************/
static void
pbTask(void *parameters)
{
	f_enterFS();		// Must make filesystem aware of this task
						// The filesystem has been initialized before this call

	while (1) {
		PBCommand cmd;
		
		// Handle new commands and set pbTask blocktime
		if (xQueueReceive(pbEventQueue, &cmd, 4) == pdTRUE) {
			switch (cmd.cmd) {
			case PB_CMD_INIT_MP3:
				if (!prepareStartMp3File(cmd.startCluster, cmd.fileSize))
					break;
				pi.iState = PBS_CueUp;
				break;
				
			case PB_CMD_INIT_WAV:
				if (!prepareStartWavFile(cmd.filename))
					break;
				pi.iState = PBS_CueUp;
				break;
				
			case PB_CMD_INIT_SIN:
				pi.iState = PBS_CueUp;
				pi.pbType = PBT_Sin;
				pi.pbStart= tmrGetMS();
				pi.file   = NULL;
				pi.pbSampleRate = cmd.sr;
				pi.frame = 0;
				pi.pbNumSamp = createSinSound();				
				audSetPlaybackRate(pi.pbSampleRate);
				break;
				
			case PB_CMD_PLAY:
				if ((pi.iState==PBS_CueUp)||(pi.iState==PBS_Pause))
					pi.iState = PBS_Play;
				break;
				
			case PB_CMD_PAUSE:
				if (pi.iState==PBS_Play)
					pi.iState = PBS_Pause;
				break;
				
			case PB_CMD_STOP:
			case PB_CMD_CLOSE_PB:
				pi.iState = PBS_Stop;
				audFlush();
				break;
			}
			
			xSemaphoreGive(audSem);				// Command is handled
		}
		
		// Handle playback of sound
		if (pi.iState == PBS_Play) {
			playback();
		} else {
			audSpeakerAmp(false);				// Disable speaker
		}
	}
}

/*******************************************************************************
 * Function:	pbExecuteAndWait
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
 * Function:	pbInitPlayMP3
 * 
 * Summary:		
 *******************************************************************************/
void
pbInitPlayMP3(LongWord startCluster, LongWord fileSize)
{
	PBCommand cmd;
	
	cmd.cmd = PB_CMD_INIT_MP3;
	cmd.startCluster = startCluster;
	cmd.fileSize = fileSize;

	pbExecuteAndWait(&cmd, 10, 10);	// Block until command is handled
}

/*******************************************************************************
 * Function:	pbInitPlayWav
 * 
 * Summary:		
 *******************************************************************************/
void
pbInitPlayWav(char *wavFile)
{
	PBCommand cmd;
	
	cmd.cmd = PB_CMD_INIT_WAV;
	cmd.filename = wavFile;
	
	pbExecuteAndWait(&cmd, 10, 10);	// Block until command is handled
}

/*******************************************************************************
 * Function:	pbInitPlaySin
 * 
 * Summary:		
 *******************************************************************************/
void
pbInitPlaySin(Word sampleRate)
{
	PBCommand cmd;
	
	cmd.cmd = PB_CMD_INIT_SIN;
	
	if (sampleRate < AD_PLAYBACK_RATE_MIN) sampleRate = AD_PLAYBACK_RATE_MIN;
	if (sampleRate > AD_PLAYBACK_RATE_MAX) sampleRate = AD_PLAYBACK_RATE_MAX;
	cmd.sr = sampleRate;
	
	pbExecuteAndWait(&cmd, 10, 10);	// Block until command is handled
}

/*******************************************************************************
 * Function:	pbPlay
 * 
 * Summary:		
 *******************************************************************************/
void
pbPlay()
{
	PBCommand cmd;
	
	cmd.cmd = PB_CMD_PLAY;	
	cmd.when = tmrGetMS();
	
	pbExecuteAndWait(&cmd, 10, 10);	// Block until command is handled
	TRACE_AUD(("audMng: Play cmd compleated after %d ms\r\n", tmrGetMS()-cmd.when);)
}

/*******************************************************************************
 * Function:	pbPause
 * 
 * Summary:		
 *******************************************************************************/
void
pbPause()
{
	PBCommand cmd;
	
	cmd.cmd = PB_CMD_PAUSE;

	pbExecuteAndWait(&cmd, 10, 10);	// Block until command is handled
}

/*******************************************************************************
 * Function:	pbStop
 * 
 * Summary:		
 *******************************************************************************/
void
pbStop()
{
	PBCommand cmd;
	
	cmd.cmd = PB_CMD_STOP;

	pbExecuteAndWait(&cmd, 10, 10);	// Block until command is handled
}

/*******************************************************************************
 * Function:	pbSetVolume
 * 
 * Summary:		
 *******************************************************************************/
void 
pbSetVolume(
	Word newVolume
){
	if (newVolume > PB_VOLUME_MAX) {
		newVolume = PB_VOLUME_MAX;
	}
	
	audSetVolume((newVolume * AD_VOLUME_MAX) / PB_VOLUME_MAX);	
}

/*******************************************************************************
 * Function:	pbInit
 * Returns:		void
 * 
 * Summary:		Call once to initilize the playback unit
 *******************************************************************************/
void
pbInit(void *bigBuffer)
{
	mp3_Decoder = bigBuffer;
	pi.iState    = PBS_Stop;
	pi.file      = NULL;
	pi.pcmBuf[0] = sampBuf0;			// Set pointers to sample buffers
	pi.pcmBuf[1] = sampBuf1;

	pbEventQueue = xQueueCreate(PB_QUEUE_SIZE, sizeof(PBCommand));   		// Create PB Work Queue
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
 * Function:	pbClose
 * Returns:		void
 * 
 * Summary:		
 *******************************************************************************/
void
pbClose()
{
	Boolean cmdHandled;
	PBCommand cmd;								
	
	// Send CLOSE-CMD to PB.
	cmd.cmd = PB_CMD_CLOSE_PB;
	cmdHandled = pbExecuteAndWait(&cmd, 10, 100);		// Block until command is handled
	ASSERT(cmdHandled, ("audMng: Unable to close playback task"));

	vTaskDelete(gPBTaskHandle);
	vQueueDelete(pbEventQueue);
	vQueueDelete(audSem);

}

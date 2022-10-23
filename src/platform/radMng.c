/* radMng.c
**
** To handle and manage the FM Radio with n slots.
** 
** Created  22-09-06	Kalle
**
** (C)Copyright Dataton Utvecklings AB 2007, All Rights Reserved.
**
*/

//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\audDrv.h"				// To do
#include "driver\radDrv.h"				// To do
#include "driver\tmrDrv.h"

#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"

#include "platform\radMng.h"

#include "platform\hccfat\api_f.h"

//---------------------------------DEFINES----------------------------------------
#define NUM_RADIO_MEMORY_SLOTS		LEDD_NUM_RING_LEDS
#define SQUELCH_SAMP_INTERV			250		// Sample 4 times every second
#define SQUELCH_VOLUME				1		// 0..15
#define MUTE_VOLUME					0		// 0..15
#define DEFAULT_SQL_LEVEL			0		// 0..40, 0=Off, 18
#define DEFAULT_RADIO_FREQ			883		// 88.3 MHz

//---------------------------------TYPEDEFS---------------------------------------
typedef struct _RadioChannelInfo
{
	Word freq;
} RadioChannelInfo;

//----------------------------GLOBAL VARIABLES------------------------------------
static RadioChannelInfo gFreqTab[NUM_RADIO_MEMORY_SLOTS];	// Radio channel list

static Word	   gSlotIndex		= 0;						
static Boolean gRadioActive		= false;		// Current state of Radio
static Boolean gIsMuted			= false;
static Boolean gIsSquelched		= false;		// Is radio squelched?
static Boolean gMono			= false;		// Mono or stereo

static Word gSquelchLevel;						// Squelch level
static Word gFreq;								// Frequency

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	UpdateRadioFile
 * Summary:		Write radio memory slots to file
 *******************************************************************************/
static void
updateRadioFile()
{
	F_FILE *raChFile = f_open("A:/DB/radio.chn", "w"); 
	if (raChFile) {
		f_write(gFreqTab, 1, sizeof(gFreqTab), raChFile);
		f_close(raChFile);
	}
}

/*******************************************************************************
 * Function:	rdmInit
 * Summary:		Initiate Radio Manager. Sets initial frequency to 88.3 Mhz.
 *				Sets radio to not active.
 *******************************************************************************/
void
rdmInit()
{
	F_FILE *raChFile;
	memset(&gFreqTab, 0, sizeof(gFreqTab));
	
	gSlotIndex	 = 0;			// First slot in table
	gRadioActive = false;
	gIsMuted	 = false;
	gIsSquelched = false;		
	gMono		 = false;		// Stereo
	
	gSquelchLevel  = DEFAULT_SQL_LEVEL;
	gFreq		   = DEFAULT_RADIO_FREQ;
	
	raChFile = f_open("A:/DB/radio.chn", "r");
	if(raChFile) {
		if(f_read(gFreqTab, 1, sizeof(gFreqTab), raChFile) < sizeof(gFreqTab)) {
			// Error, annoying but not serious, clear frequency table
			memset(&gFreqTab, 0, sizeof(gFreqTab));
		}
		f_close(raChFile);
	}
}

/*******************************************************************************
 * Function:	rdmClose
 * Summary:		Close Radio Manager
 *******************************************************************************/
void
rdmClose()
{
	rdmDisable();
}

/*******************************************************************************
 * Function:	rdmEnable
 * Summary:		Enable Radio Manager
 * NOTE:		Radio can't be active when calling this function
 *******************************************************************************/
void
rdmEnable()
{
	if (!gRadioActive) {
		gRadioActive = true;				// Radio is active
		rddInit();							// Initate Radio driver.
		audEnableRadio();					// Enable DAC analog input for audio from radio
		audRegUser(EADU_RadioManager);		// Register RM as an AD user
		gIsMuted = false;					// Radio isn't muted
		gIsSquelched = false;				// Radio isn't squelched
		rddSetVol(0);						// Set volume to 0
		rdmSetFreq(gFreq, false);			// Set frequency
		rdmMono(gMono);						// Set mono/stereo		
		rddSetVol(15);						// Set volume to 15
	}
}

/*******************************************************************************
 * Function:	rdmDisable
 * Summary:		Disable Radio Manager
 * NOTE:		Radio must be active when calling this function
 *******************************************************************************/
void
rdmDisable()
{
	if (gRadioActive) {
		gFreq = rddGetFreq();				 // Save current frequency
		audUnregUser(EADU_RadioManager);	 // Unregister RM as an AD user
		audDisableRadio();					 // Disable DAC analog input for audio from radio
		rddSleep();							 // Set Radio to sleep
		gRadioActive = false;				 // Radio is no longer active
	}
}

/*******************************************************************************
 * Function:	rdmSleep
 * Summary:		Sleep radio if active.
 *******************************************************************************/
void
rdmSleep()
{
	if (gRadioActive) {
		rddSleep();
	}
}

/*******************************************************************************
 * Function:	rdmWake
 * Summary:		Wake radio if active.
 *******************************************************************************/
void
rdmWake()
{
	if (gRadioActive) {
		rddWake();
	}
}

/*******************************************************************************
 * Function:	rdmMaint
 * Summary:		Checks current reception level and checks if radio is squelched.
 *				Sets volume to 0 if radio is squelshed or muted.
 *******************************************************************************/
void
rdmMaint()
{
	static Word currReceptionLevel;
	static LongWord lastMaintCall = 0;			// When maint was last called
	
	Boolean prevSquelch;
	Word currentSamp;
	
	if (gRadioActive && !gIsMuted && gSquelchLevel &&
	    TM_GT(tmrGetMS(), lastMaintCall)) {
	    
		lastMaintCall = tmrGetMS() + SQUELCH_SAMP_INTERV;

		prevSquelch = gIsSquelched;
		currentSamp = rddGetSignalStrength();
		
		currReceptionLevel = (currReceptionLevel * 3 + currentSamp) / 4;
		gIsSquelched = currReceptionLevel < gSquelchLevel;
		
		// Update radio mute
		if (prevSquelch != gIsSquelched) {
			if (gIsMuted)
				rddSetVol(MUTE_VOLUME);
			else if (gIsSquelched)
				rddSetVol(SQUELCH_VOLUME);
			else
				rddSetVol(15);
		}
		
		TRACE_RAD(("RDM samp=%d  level=%d  limit=%d  isSql=%d\n\r", 
								currentSamp, 
								currReceptionLevel, 
								gSquelchLevel, 
								gIsSquelched);)
	}
}

/*******************************************************************************
 * Function:	rdmIsRadioActive
 * Summary:		Returns TRUE if radio is active.
 *******************************************************************************/
Boolean
rdmIsRadioActive()
{
	return gRadioActive;
}

/*******************************************************************************
 * Function:	rdmMute
 * Summary:		Sets volume to 0 if activate-value is TRUE or radio is squelched.
 *******************************************************************************/
void
rdmMute(Boolean activate)				// if TRUE, set volume to 0
{
	gIsMuted = activate;

	if (gRadioActive) {
		TRACE_RAD(("rdmMute(%d)\n\r", gIsMuted);)
		if (gIsMuted)
			rddSetVol(MUTE_VOLUME);
		else if (gIsSquelched)
			rddSetVol(SQUELCH_VOLUME);
		else
			rddSetVol(15);
	}
}

/*******************************************************************************
 * Function:	rdmIsMuted
 * Summary:		Returns TRUE if radio is muted
 *******************************************************************************/
Boolean
rdmIsMuted()
{
	return gIsMuted;
}

/*******************************************************************************
 * Function:	rdmMono
 * Summary:		Sets radio to mono sound if activate is TRUE.
 *******************************************************************************/
void
rdmMono(Boolean activate)				// if TRUE, set radio to mono
{
	gMono  = activate;

	if (gRadioActive) {
		rddMono(activate);
	}
}

/*******************************************************************************
 * Function:	rdmSetSquelchLevel
 * Summary:		Set squelch level.
 *******************************************************************************/
void
rdmSetSquelchLevel(Word level)			// New level for squelching
{
	gSquelchLevel = level;
	
	if (level == 0) {					// We are leaving squelch mode
		gIsSquelched = false;
		
		if (gIsMuted)
			rddSetVol(MUTE_VOLUME);
		else
			rddSetVol(15);
	}
}

/*******************************************************************************
 * Function:	rdmIsSquelched
 * Summary:		Returns TRUE if radio is squelched
 *******************************************************************************/
Boolean
rdmIsSquelched() {
	return gIsSquelched;
}

/*******************************************************************************
 * Function:	rdmSeek
 * Summary:		Seek frequency upwards or downwards
 *******************************************************************************/
void
rdmSeek(
	Boolean up,			// if TRUE seek upwards, if false seek downwards
	Boolean save		// Save frequency?
){
	if (gRadioActive) {
		gIsSquelched = false;			// Always unsquelch when switching radio freq
		rdmMute(rdmIsMuted());			// Update volume
		
		rddSeek(up);									 
		if (save) {									 	// Save frequency
			gFreqTab[gSlotIndex].freq = rddGetFreq();	// Store into freq. table
			updateRadioFile();						 	// Update radio file
		}
	}
}

/*******************************************************************************
 * Function:	rdmSetFreq
 * Summary:		Set frequency to radio driver in (100KHz)-value.
 *******************************************************************************/
void
rdmSetFreq(
	LongWord freqIn100KHz,				// Set radio to this frequency
	Boolean save						// Indicates if we should save the freq. to file
){
	if (gRadioActive) {
		gIsSquelched = false;			// Always unsquelch when switching radio freq
		rdmMute(rdmIsMuted());		// Update volume
		
		rddSetFreq((Word)(freqIn100KHz));
		
		if (save) {
			gFreqTab[gSlotIndex].freq = rddGetFreq();
			updateRadioFile();		
		}
	}
}

/*******************************************************************************
 * Function:	rdmSaveSlotInfo
 * Summary:		Save current frequency and update the radio file (if persistent)
 *******************************************************************************/
void
rdmSaveSlotInfo(
	 Boolean persistent		// Update radio file if TRUE
){
	if (gRadioActive) {
		gFreqTab[gSlotIndex].freq = rddGetFreq();
		if (persistent) {
			updateRadioFile();		
		}
	}
}

/*******************************************************************************
 * Function:	rdmGetTunedFreq
 * Summary:		Get current frequency.
 *******************************************************************************/
Word
rdmGetTunedFreq()
{
	if (gRadioActive) {
		return rddGetFreq();
	} else
		return 0;
}

/*******************************************************************************
 * Function:	rdmGetSlotFreq
 * Summary:		Get frequency at a certain slot in freq.table
 *******************************************************************************/
Word
rdmGetSlotFreq()
{
	return gFreqTab[gSlotIndex].freq;
}

/*******************************************************************************
 * Function:	rdmNextSlot
 * Summary:		Get frequency at next slot in freq.table
 *******************************************************************************/
Word
rdmNextSlot()
{	
	if(gSlotIndex >= NUM_RADIO_MEMORY_SLOTS-1)
		return (gSlotIndex = 0);					// Start over
	else 
		return (gSlotIndex = (gSlotIndex + 1));
}

/*******************************************************************************
 * Function:	rdmPrevSlot
 * Summary:		Get frequency at previous slot in freq.table
 *******************************************************************************/
Word
rdmPrevSlot()
{	
	if(gSlotIndex == 0)
		return (gSlotIndex = NUM_RADIO_MEMORY_SLOTS-1);
	else
		return (gSlotIndex = (gSlotIndex - 1));			
}

/*******************************************************************************
 * Function:	rdmSlotIndex
 * Summary:		Get slot index in freq. table
 *******************************************************************************/
Word
rdmSlotIndex()
{
	return gSlotIndex;
}

/*******************************************************************************
 * Function:	rdmNumSlots
 * Summary:		Returns the number of slots
 *******************************************************************************/
Word
rdmGetNumSlots()
{
	return NUM_RADIO_MEMORY_SLOTS;
}

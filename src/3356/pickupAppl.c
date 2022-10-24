/* pickupAppl.c - Dataton 3356 Pickup
**
**	Pickup guide application
** 		
**
** Created 10-10-30	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
**
**/

//---------------------------------INCLUDES---------------------------------------
#include <stdio.h>

#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "driver\audDrv.h"
#include "driver\cpuDrv.h"
#include "driver\hpcDrv.h"
//#include "driver\ledDrv.h"
#include "driver\hccfat\mmc.h"

#include "platform\audMng.h"
#include "platform\evtMng.h"
#include "platform\ledMng.h"
#include "platform\radMng.h"
#include "platform\setMng.h"
#include "platform\FreeRTOS\FreeRTOS.h"
#include "platform\FreeRTOS\task.h"
#include "platform\hccfat\api_f.h"
#include "platform\hccfat\port_f.h"
#include "platform\utilities\pubFileUtil.h"

#include "3356\pickupAppl.h"
#include "3356\cacheHdlr.h"

//--------------------------------FUNCTIONS---------------------------------------
static void
infoOut(F_FILE *fp, char *text)
{
	if (fp != NULL)
		f_write(text, 1, strlen(text), fp);
	else
		TRACE_GDE((text);)
}

/*******************************************************************************
 * Function:	firmwareInfo
 * Summary:		Send firmware info to file or debug port
 *******************************************************************************/
void
firmwareInfo(Boolean toFile)
{
	char line[80];
	prdInfo *ai = prdInfoGet();
	F_FILE *fp = NULL;
	
	if (toFile)
		fp = f_open("A:/DB/id.txt", "w");
	
	sprintf(line, "*----------------------------*\r\n");
	infoOut(fp, line);
	sprintf(line, "* 3356  PICKUP FIRMWARE      *\r\n");
	infoOut(fp, line);
	sprintf(line, "*                            *\r\n");
	infoOut(fp, line);
	
	if (ai->hwVer < 0x0100)
		sprintf(line, "* Hardware ver: %d.%d.%d        *\r\n", (ai->hwVer/100)%10, (ai->hwVer/10)%10, ai->hwVer%10);
	else
		sprintf(line, "* Hardware ver: %d.%d.%d        *\r\n", (ai->hwVer>>16)%256, (ai->hwVer>>8)%256, ai->hwVer%256);
	infoOut(fp, line);

	if (ai->blVer < 0x0100)
		sprintf(line, "* Bootload ver: %d.%d.%d        *\r\n", (ai->blVer/100)%10, (ai->blVer/10)%10, ai->blVer%10);
	else
		sprintf(line, "* Bootload ver: %d.%d.%d        *\r\n", (ai->blVer>>16)%256, (ai->blVer>>8)%256, ai->blVer%256);
	infoOut(fp, line);

	sprintf(line, "* Software ver: %d.%dB%d        *\r\n", (ai->swVer>>16)%256, (ai->swVer>>8)%256, ai->swVer%256);
	infoOut(fp, line);
	sprintf(line, "* SerialNumber: %5d        *\r\n", ai->serialNumber);
	infoOut(fp, line);
	sprintf(line, "*                            *\r\n");
	infoOut(fp, line);
	sprintf(line, "*----------------------------*\r\n");
	infoOut(fp, line);
	
	if (fp != NULL)
		f_close(fp);
}

/*******************************************************************************
 * Function:	dispatchKBEvent
 * Summary:		Dispatch key board event
 *******************************************************************************/
static void
dispatchKBEvent(EMEvent *evt)
{
	optFile file;
	
	switch (evt->sw.event) {
	case ESWE_KeyDown:
		switch (evt->sw.key) {
		case ESWK_KeyAction:
			TRACE_GDE(("ESWK_KeyAction\r\n");)
			if (CacheLookup(1234, true, &file)) {
				pbInitPlayMP3(file.startCluster, file.fileSize);
				pbPlay();
			}
			pbInitPlayWav("INPUT.WAV");
			pbPlay();
			break;
			
		case ESWK_KeyLeft:
			TRACE_GDE(("ESWK_KeyLeft\r\n");)
			pbInitPlaySin(44100);
			pbPlay();
			break;
			
		case ESWK_KeyRight:
			TRACE_GDE(("ESWK_KeyRight\r\n");)
			pbStop();
			break;
		}
		break;		// ESWE_KeyDown
		
	case ESWE_KeyUp:
		break;
	}
}

/*******************************************************************************
 * Function:	ls
 * Summary:		Scan a directory (recursively)
 *******************************************************************************/
static void 
ls(
	char *tmp_dir	// Directory to scan
){
	F_FIND find;
	
	f_chdir(tmp_dir);			// Move to directory
	f_getcwd(tmp_dir, 256);		// Get full path
	
	TRACE_STM(("Current working directory: %s\n\r", tmp_dir);)
	
	if (!f_findfirst("*.*", &find))	// Find first object in directory (probably ".")
	{
		do
		{
			TRACE_STM(("filename:%s", find.filename);)
			
			if (strncmp(find.filename, ".", 1) == 0 || find.attr & F_ATTR_HIDDEN)	// Hidden files
			{
				TRACE_STM((" hidden\n\r");)
			}
			else if (find.attr & F_ATTR_DIR)										// Directory
			{
				TRACE_STM(("\n\r");)
				strcpy(tmp_dir, find.filename);
				ls(tmp_dir);														// List directory
				f_chdir("..");												// Go back
				f_getcwd(tmp_dir, 256);										// Get full path
				TRACE_STM(("Current working directory: %s\n\r", tmp_dir);)
			}
			else
			{
				TRACE_STM((" size %d\n\r", find.filesize);)				// Ordinary file
			}
		} while(!f_findnext(&find));
	}
}

/*******************************************************************************
 * Function:	openFileSystem 
 * Summary:		Opens the file system and set volyme lable to "PICKUP"
 *				if necessary
 *******************************************************************************/
static void
openFileSystem(){
	// Initialize filesystem for the main thread
	int ret = f_enterFS();
	
	// Initialize volume
	ret = f_initvolume(0, mmc_initfunc, F_AUTO_ASSIGN);
	ASSERT(ret == 0, ("F INIT VOLUME FAILED: %d",ret));
	
	/* In non-multitask and multitask systems, if the user needs realtive
	   path accessing then f_chdrive must be issued. In non-multitask system
	   after f_initvolume, in a multitask system every f_enterFS must be followed
	   with an f_chdrive function call. In a multitask system every task has its
	   individual current drive. 
	*/
	ret = f_chdrive(0);
	ASSERT(ret == 0, ("F CHANGE DRIVE FAILED: %d", ret));
	
	// Set label if it doesn't exists (ie. "NO NAME")
	{
		char label[12];
		if(f_getlabel(0,label,12)) {
			TRACE_GDE(("Couldn't fetch label on disk\r\n");)		
		}
		else {
			if((strlen(label) == 0) || 
			   (strncmp(label, "NO NAME", 7) == 0) ||
			   (strncmp(label, "Kingston", 8) == 0)) 
			{
				if(!f_setlabel(0,"PICKUP")) {
					TRACE_GDE(("Label on disk has been changed to PICKUP.\r\n");)
					f_getlabel(0,label,12);
				}
				else {
					TRACE_GDE(("Failed to change label on disk\r\n");)
				}
			}
		}
	}
	
	// Make DB directory (hidden)
	{
		F_FIND find;
		if(f_findfirst("A:/DB", &find)) {
			f_mkdir("A:/DB");
		}
		f_setattr("A:/DB", F_ATTR_HIDDEN);
	}
	
	// Wait 10 ms
	vTaskDelay(10);
}

/*******************************************************************************
 * Function:	closeFileSystem
 * Summary:		Will close the file system
 *******************************************************************************/
static void
closeFileSystem()
{
	/* Delete volume */
	int ret = f_delvolume(0);
	ASSERT(!ret, ("f_delvolume failed: %d", ret));
	
	/* Release main thread from filesystem */
	f_releaseFS(fn_gettaskID());
}

/*******************************************************************************
 * Function:	pickupClose
 * Summary:		
 *******************************************************************************/
static void
pickupClose()
{
	Word idx;
	
	for (idx=0; idx < LEDD_NUM_RING_LEDS+1; idx++)
		ledSetLedLevel(idx, LEDD_LEVEL_MIN);

	rdmClose();
	pbClose();
	audClose();
	CacheClose();
	
	closeFileSystem();
}

/*******************************************************************************
 * Function:	pickupInit
 * Summary:		
 *******************************************************************************/
static void
pickupInit(void *bigBuffer)
{
	TRACE_GDE(("Guide application \r\n");)

	openFileSystem();
	if (Need2UpdateFirmware(prdInfoGet()->swVer)) {
		cpuForceWatchdogReset();	// Will enter boot loader to update firmware
	}
	
	firmwareInfo(true);				// Firmware info to file
	CacheInit(true);
	
	audInit();
	pbInit(bigBuffer);
	rdmInit();
	
	pbSetVolume(30);
	audSpeakerAmp(false);			// Disable speaker
}

/*******************************************************************************
 * Function:	pickupAppl
 * Summary:		
 *******************************************************************************/
void
pickupAppl(void *bigBuffer)
{
	EMEvent evt;
	
	pickupInit(bigBuffer);
	ledDisplay(kLedCenterBlink);
	ls("A:/");

	do {
		cpuKickWatchDog();
		vTaskDelay(10);

		if (EM_GetNextEvent(&evt, 0)) {
			switch (evt.source) {
			case KEVS_Keyboard:	dispatchKBEvent(&evt);
				break;
			}
		}
		
		ledMaint();
		rdmMaint();
	} while (hpcSense() != EHPC_USB);
	
	pickupClose();
}







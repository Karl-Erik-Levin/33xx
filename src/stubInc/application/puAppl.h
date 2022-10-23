/* puAppl.h
**
** (C)Copyright Dataton Utvecklings AB 2005, All Rights Reserved
**
** History:
** 2005-08-22 		Fred J			Created
*/
#ifndef _PUAPPL_H_
#define _PUAPPL_H_
//---------------------------------INCLUDES---------------------------------------
#include "Platform/FreeRTOS/SignalPool.h"
//#include "TIDManager.h"
//----------------------------------DEFINES---------------------------------------
#define NUM_EQ	(5)				// 5 Equalizers
//---------------------------------TYPEDEFS---------------------------------------
typedef enum					
{	
	EAM_AudioFiles,				// Playing audio files
	EAM_Radio,					// Playing radio
	EAM_USB						// Connected to USB
} EAppMode;						

typedef enum
{
	ERSM_Use,
	ERSM_Program
} ERadioSubMode;

typedef enum
{
	EAUSM_Transponder,			// Playing audio files related to a transponder
	EAUSM_Cafe,					// Playing audio files with no transponder
	EAUSM_SetEQ
} EAudioFileSubMode;

typedef enum
{
	ETAM_Off,
	ETAM_On,
	ETAM_Locate
} TransponderAccessMode;

typedef enum
{
	ENM_OneButton,
	ENM_Classic
} ENavigationMode;

typedef enum
{
	ELA_Off,
	ELA_Prev,
	ELA_EQ,
	ELA_Play,
	_ELA_NumActions
} ELeftAction;					// Left button action

typedef enum
{
	ERA_Off,
	ERA_Next,
	ERA_FM,
	ERA_Play,
	_ERA_NumActions
} ERightAction;					// Right button action

typedef enum
{
	EPSL_None,
	EPSL_Half,					// Will be able to wake from IR
	EPSL_Full,
	_EPSL_NumPowerSavelLevels
} EPowerSaveLevel;

typedef enum
{
	EASM_On,
	EASM_Off,
	EASM_AfterClick
} EAutoSynchMode;
//----------------------------GLOBAL VARIABLES------------------------------------
static const LongWord KShowProgessDurationMs = 800;
static const Word PROGRESS_LEVEL_MAX = 12;
//-------------------------------DEFINITIONS--------------------------------------
void puAppl_Init();

Boolean puAppl_Lock();
void puAppl_Release();

void puAppl_DisplayMode(Boolean force);
void puAppl_SetMode(EAppMode inMode);

Boolean puAppl_SetLevel(Word inLevel, Boolean flash);

void puAppl_ToggleMode();
EAppMode puAppl_GetMode();
void puAppl_SetMode(EAppMode newMode);

EAudioFileSubMode puAppl_GetAudioFileSubMode();
EAudioFileSubMode puAppl_GetPrevAudioFileSubMode();
void puAppl_SetAudioFileSubMode(EAudioFileSubMode newSubMode);

void puAppl_ResetEQ();
void puAppl_SetEQ(Word newEQ);
Word puAppl_GetEQ();
void puAppl_NextEQ();
void puAppl_PrevEQ();

TransponderAccessMode GetTransponderAccess();

/*void EnableRadio();
void DisableRadio();
Boolean RadioActive();
*/

ERadioSubMode puAppl_GetRadioSubMode();
void puAppl_SetRadioSubMode(ERadioSubMode newSubMode);

xSignalPoolHandle GetAppSignalPool();

void puAppl_SetRadioCounterRel(int inDirection);
void puAppl_GetRadioCounter();

//CacheStream *GetTIDHandle();
Word GetActiveHSID();

#endif 		// _PUAPPL_H_


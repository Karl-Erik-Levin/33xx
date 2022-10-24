/* cacheHdlr.h - Dataton 3356 Pickup
**
**	Cache file handler
**
**	Create cache file, or a small database, of all mp3 file on Pickup
**	flash card. Purpose of this is to speed up finding matching mp3 
**	file from a transponder number. This module also handle filter
**	for selecting mp3 files with requested language. 
** 		
**
** Created 10-10-30	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
**
*/

#ifndef _cacheHdlr_
#define _cacheHdlr_


//-------------------------------- INCLUDES ---------------------------------------
#include "MFLib\Dataton_Types.h"

//-------------------------------- TYPEDEFS ---------------------------------------
typedef struct _optFile {
	LongWord	startCluster;
	LongWord	fileSize;
} optFile;

//-------------------------------- PROTOTYPS ---------------------------------------
void CacheInit(Boolean rebuildCache);
void CacheClose(void);
Boolean CacheLookup(Word transponder, Boolean executeFileCmd, optFile *file);


#endif	// _cacheHdlr_

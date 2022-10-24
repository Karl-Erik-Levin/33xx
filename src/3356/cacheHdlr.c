/* cacheHdlr.c - Dataton 3356 Pickup
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
**/

//--------------------------------- INCLUDES ---------------------------------------
#include <ctype.h>
#include "MFLib\Dataton_Types.h"
#include "MFLib\TraceAssert.h"

#include "platform\hccfat\api_f.h"

#include "3356\cacheHdlr.h"
#include "3356\ID3Parser.h"

//--------------------------------- DEFINES ----------------------------------------
#define FOLDERLEVEL_TO_SCAN			4
#define MAX_GENRE_NAME_LENGTH      16
#define MAX_GENRES                 32
#define MAX_ACTIVE_GENRES           8

#define ROOT_DIR "A:/"
#define DB_DIR  ROOT_DIR "DB2/"
#define TRACK_DB_FILE_NAME DB_DIR "TRACKS.DB"
#define GENRE_DB_FILE_NAME DB_DIR "GENRES.DB"

//-------------------------------- TYPES -------------------------------------
typedef struct _cacheEntry {
	optFile fileInfo;
	Word transponderId;
	Word genreId;
	char genre[MAX_GENRE_NAME_LENGTH];
} CacheEntry;

typedef struct _genreEntry {
	char genreName[MAX_GENRE_NAME_LENGTH];
	Word genreId;
} GenreEntry;

typedef struct _activeGenresTab {
	Boolean allActive;
	Word nOfActiveGenres;    
	Word activeGenres[MAX_ACTIVE_GENRES];
} ActiveGenresTab;

//-------------------------------- GLOBALS -------------------------------------
static GenreEntry gGenres[MAX_GENRES];
static ActiveGenresTab gActiveGenres;

//-------------------------------- PROTOTYPS ----------------------------------
static void cacheRebuild();
static Boolean skipFile(F_FIND* ffind);
static void addToDb(char* fileName);

/*******************************************************************************
 * Function:	strnicmp
 * Returns:		  0 if lhs is equal to rhs
 *				  1 if lhs is not equal than rhs
 * Summary:		Compares both strings. Ignore case. Max n number of characters.
 *******************************************************************************/
#ifdef KALLE_MW
  int __cdecl strnicmp(const char *lhs, const char *rhs, unsigned int cnt)
#else
  int strnicmp(char *lhs, char *rhs, int cnt)
#endif
{
	while (cnt--)
	{
		if (toupper(*lhs) != toupper(*rhs) || *rhs == 0 && *lhs != 0 || *lhs == 0 && *rhs != 0)
			return 1;

		lhs++;
		rhs++;
	}
	return 0;
}


/*******************************************************************************
 * Function:	isMp3File
 * Summary:		Maybe should this function open file and look for mp3 frame 
 *				header
 *******************************************************************************/
static Boolean
isMp3File(char *fileName)
{
	int len = strlen(fileName);
		
	if ( len < 4 ) {
		return false;
	}
	
	if (strnicmp(&fileName[len - 4], ".mp3", 4) == 0)
		return true;	
		
	return false;
}


/*******************************************************************************
 * Function:	CacheInit
 * Summary:		Open cahce file. If parameter rebuildCache is true a new scan
 *				of the file tree is be performed and a new cache file is created
 *******************************************************************************/
void
CacheInit(Boolean rebuildCache)
{
	if ( rebuildCache ) {
		cacheRebuild();
	}
	
	// TODO: open cache etc. 
			
}


/*******************************************************************************
 * Function:	CacheClose
 * Summary:		Deallocate resources
 *******************************************************************************/
void
CacheClose(void)
{

}


/*******************************************************************************
 * Function:	CacheLookup
 * Summary:		Return true if a file is found and parameters for open file
 *				is returned in parameter *file
 *******************************************************************************/
Boolean
CacheLookup(Word transponder, Boolean executeFileCmd, optFile *file)
{
	F_FIND find;
	Boolean fileFound = false;
	
	file->startCluster = 0;
	file->fileSize = 0;
	
	if(!f_findfirst("*.*", &find))			// Find first file in directory
	{
		do {								// Loop through every file/directory
			if (isMp3File(find.filename)) {
				file->startCluster = find.cluster;
				file->fileSize = find.filesize;
				fileFound = true;
			}
		} while(!fileFound && !f_findnext(&find));
	}
	
	return fileFound;
}


/*******************************************************************************
 * Function:	cacheRebuild
 * Summary:		Traverses the directory tree and parses ID3-tags in all mp3
 *              files found, and stores them in a database. 
 *******************************************************************************/
static void
cacheRebuild(char* sourceDir, int depth)
{
	F_FIND ffind;
	
	if (depth > FOLDERLEVEL_TO_SCAN) {
		return;
	}
	
	f_chdir(sourceDir);
	
	if (f_findfirst(ROOT_DIR "*.*", &ffind) == 0) {
		do {
			if (skipFile(&ffind)) {
				continue;
			}
			if ((ffind.attr & F_ATTR_DIR) != 0) {
				cacheRebuild(ffind.filename, depth + 1);
			}
			if (isMp3File(ffind.filename)) {
				addToDb(ffind.filename);
			}
		} while (f_findnext(&ffind) == 0);
	}				
	f_chdir("..");		
}


/*******************************************************************************
 * Function:	skipFile
 * Summary:		
 *               
 *******************************************************************************/
static Boolean
skipFile(F_FIND* ffind)
{
	return (ffind->filename[0] == '.') ||
	       ((ffind->attr & F_ATTR_HIDDEN) != 0);
}


/*******************************************************************************
 * Function:	addToDb
 * Summary:		
 *               
 *******************************************************************************/
static void
addToDb(char* fileName)
{
	ID3Stream id3Str;
	
	if (!ID3Open(fileName, &id3Str)) {
		return;
	}
	
	while (ID3Next(&id3Str)) {
		if (strnicmp(id3Str.tag.tagId, "TCON", 4) == 0) {
		}
	}		
}



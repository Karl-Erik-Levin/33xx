/* mp3.h
**
** 
**
** (C)Copyright Dataton Utvecklings AB 2007, All Rights Reserved.
**
** History:
** 20070410			Fred J			Created
**/
#ifndef _mp3_H_
#define _mp3_H_
//---------------------------------INCLUDES---------------------------------------
#include "Platform\hccfat\api_f.h"
//-------------------------------DEFINITIONS--------------------------------------
Word MP3_BytesPerMS(Word inKBPS);
long MP3_FindFirstFrame(F_FILE *fp);
long MP3_FindFrame(F_FILE *fp, unsigned int fromPosition);

#endif		// _mp3_H_

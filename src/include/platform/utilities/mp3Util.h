/* mp3Util.h - Dataton 3356 Pickup
**
** This file contains helper functions to allow a user to easier interact and interface
** to MP3 data streams. Some it comes from the PU-classic source tree.
**
**
** Created 10-10-30	Kalle
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
*/
#ifndef _mp3Util_
#define _mp3Util_


//--------------------------------PROTOTYPS---------------------------------------
Word MP3_BytesPerMS(Word inKBPS);
long MP3_FindFirstFrame(F_FILE *fp);
long MP3_FindFrame(F_FILE *fp, LongWord fromPosition);


#endif	// _mp3Util_

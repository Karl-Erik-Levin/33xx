/* ID3Parser.h - Dataton 3356 Pickup
**
** Parser for ID3-tags in mp3 files
**
** Created 2010-11-12	Erik R
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
**
**/


#ifndef _id3parser_
#define _id3parser_


#include "MFLib\Dataton_Types.h"
#include "platform/hccfat/api_f.h"



typedef struct ID3Tag_ {
	char tagId[4];
	LongWord size;
	Word flags;
	LongWord dataStartPos;
} ID3Tag;



typedef struct ID3Stream_ {
	F_FILE* file;
	Word version;
	LongWord size;
	Byte flags;
	ID3Tag tag;
} ID3Stream;



Boolean ID3Open( char* fileName, ID3Stream* id3Str );
Boolean ID3Close( ID3Stream* id3Str );
Boolean ID3Next( ID3Stream* id3Str );
Boolean ID3ReadTagData( ID3Stream* id3Str, Byte* buf, Word bufSize );



#endif

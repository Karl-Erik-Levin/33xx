/* ID3Parser.c - Dataton 3356 Pickup
**
** Parser for ID3-tags in mp3 files
**
** Created 2010-11-12	Erik R
**
** (C) Copyright Dataton AB 2010, All Rights Reserved
**
**/


#include "3356\ID3Parser.h"
#include "platform\hccfat\api_f.h"

#define MAX_BYTES_TO_CHECK 100
#define ID3_HEADER_SIZE 10
#define EXTENDED_HEADER_SIZE 6
#define ID3_ID "ID3"


#define HEADER_FLAGS_UNSYNCHRONIZATION ( 1 << 7 )
#define HEADER_FLAGS_EXTENDED_HEADER   ( 1 << 6 )
#define HEADER_FLAGS_EXPERIMENTAL      ( 1 << 5 )
#define HEADER_FLAGS_FOOTER_PRESENT    ( 1 << 4 )

static Boolean LocateHeader( ID3Stream* id3Str, LongWord fileSize );
static Boolean SkipExtendedHeader( ID3Stream* id3Str );
static LongWord SyncsafeToNormal( Byte * b );


Boolean ID3Open( char* fileName, ID3Stream* id3Str )
{
	LongWord fileSize;
	
	id3Str->file = f_open( fileName, "r" );
	if ( id3Str->file == 0 ) {
		return false;
	}
	
	fileSize = f_getfilesize( id3Str->file );
	if ( fileSize < 1 ) {
		return false;
	}
	
	
	if ( !LocateHeader( id3Str, fileSize ) ) {
	 	return false;
	}
	
	// Check for extended header, and skip if necesssary
	if ( ( id3Str->flags & HEADER_FLAGS_EXTENDED_HEADER ) != 0 ) {
		if ( !SkipExtendedHeader( id3Str ) ) {
			return false;
		}
	}
	
	// Header successfully read, and we're positioned at the first tag.
	return true;
}
	
	
Boolean ID3Close( ID3Stream* id3Str ) 
{
	return f_close( id3Str->file ) == F_NO_ERROR;
}


Boolean ID3Next( ID3Stream* id3Str )
{
	Byte hdr[10];
	Word res;
	
	res = f_read( (void*)hdr, sizeof( hdr ), 1, id3Str->file );
	if ( res != sizeof( hdr ) ) {
		return false;
	}
	
	if ( f_tell( id3Str->file ) >= id3Str->size ) {
		return false;
	}
	
	memcpy( &id3Str->tag.tagId, hdr, 4 );    // Copy tag id
	id3Str->tag.size = SyncsafeToNormal( &hdr[4] );
	memcpy( &id3Str->tag.flags, &hdr[8], 2 );
	id3Str->tag.dataStartPos = f_tell( id3Str->file );
	
	return true;
}


Boolean ID3ReadTagData( ID3Stream* id3Str, Byte* buf, Word bufSize )
{
	long prevPos;
	int res;
	Word bytesToRead;
	
	prevPos = f_tell( id3Str->file );
	if ( prevPos == -1 ) {
		return false;
	}
	
	res = f_seek( id3Str->file, id3Str->tag.dataStartPos, F_SEEK_SET );
	if ( res != F_NO_ERROR ) {
		return false;
	}
	
	bytesToRead = bufSize < id3Str->tag.size ? bufSize : id3Str->tag.size;
	res = f_read( (void*)buf, bytesToRead, 1, id3Str->file );
	if ( res != bytesToRead ) {
		return false;
	}
	
	res = f_seek( id3Str->file, prevPos, F_SEEK_SET );
	if ( res != F_NO_ERROR ) {
		return false;
	}
	
	return true;
}
		


static Boolean LocateHeader( ID3Stream* id3Str, LongWord fileSize )
{
	Byte buf[10];
	int res;
	LongWord maxBytesToCheck;
	
	maxBytesToCheck = ( fileSize > MAX_BYTES_TO_CHECK ) ? MAX_BYTES_TO_CHECK : fileSize;
	
	res = f_read( (void*)&buf[1], sizeof( buf ) - 1, 1, id3Str->file );
	if ( res != sizeof( buf ) - 1 ) {
		return false;
	}
	
	while ( f_tell( id3Str->file ) < maxBytesToCheck ) {
		memmove( buf, buf + 1, sizeof( buf ) - 1 );
		res = f_getc( id3Str->file );
		if ( res == -1 ) {
			return false;
		}
		buf[sizeof( buf ) - 1] = (Byte)res;
		
		if ( memcmp( buf, ID3_ID, 3 ) == 0 ) {
			// Probably found an id3 tag, check version
			if ( ( buf[3] == 0x04 ) && ( buf[4] != 0xff ) ) {
				// Found an id3 v2.4 tag. Get flags and header size
				id3Str->flags = buf[5];
				id3Str->version = ( buf[3] << 8 ) | buf[4];
				id3Str->size = SyncsafeToNormal( &buf[6] );
				return true;
			}
		}
	}
	
	return false;
}
				
				
static Boolean SkipExtendedHeader( ID3Stream* id3Str )
{
	Byte buf[EXTENDED_HEADER_SIZE];
	Word res;
	Word size; 
	Word bytesToSkip;
	
	res = f_read( (void*)buf, sizeof( buf ), 1, id3Str->file );
	if ( res != sizeof( buf ) ) {
		return false;
	}
	
	size = SyncsafeToNormal( &buf[0] );
	bytesToSkip = size - EXTENDED_HEADER_SIZE;
	
	res = f_seek( id3Str->file, bytesToSkip, F_SEEK_CUR );
	if ( res != F_NO_ERROR ) {
		return false;
	}
	
	return true;
}
	
	
				
		
static LongWord SyncsafeToNormal( Byte * b )
{
	return ( ( b[3] & 0x7f ) <<  0 ) + 
	       ( ( b[2] & 0x7f ) <<  7 ) + 
	       ( ( b[1] & 0x7f ) << 14 ) + 
	       ( ( b[0] & 0x7f ) << 21 );
 }

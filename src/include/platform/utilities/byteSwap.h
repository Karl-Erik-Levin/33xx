/* byteSwap.h - Byte swapping routines

Defines macros for doing byte-swapping on 16, 32 and 64 bit quantities.

These macros swap between network order and the hardware order of the target 
runtime, as determined by the TARGET_RT_LITTLE_ENDIAN and TARGET_RT_BIG_ENDIAN
preprocessor constants. These constants are assumed to be defined by a previous
include file. Since we want to be target intependent here, I didn't want to define
any target-specific include file name, so I left this up to the caller. An error
will be printed if these macros aren't defined.

© Copyright Dataton AB 1999, All Rights Reserved

Created: 99-05-27 by Mike Fahl

*/

#ifndef ByteSwap_H
#define ByteSwap_H


#if !defined(TARGET_RT_LITTLE_ENDIAN) || !defined (TARGET_RT_BIG_ENDIAN)
	#error "Endianness not defined"
#endif

#if TARGET_RT_LITTLE_ENDIAN		// Non-network order style (eg, Intel)
	#define LE2H16(x) x
	#define LE2H32(x) x
	#define LE2H64(x) x
	#define H2LE16(x) x
	#define H2LE32(x) x
	#define H2LE64(x) x
	#define BE2H16(x) _swap16(x)
	#define BE2H32(x) _swap32(x)
	#define BE2H64(x) _swap64(x)
	#define H2BE16(x) _swap16(x)
	#define H2BE32(x) _swap32(x)
	#define H2BE64(x) _swap64(x)
	#define HTONS(x) _swap16(x)
	#define NTOHS(x) _swap16(x)
	#define HTONL(x) _swap32(x)
	#define NTOHL(x) _swap32(x)
	#define MAKE_FOURCC(ch0, ch1, ch2, ch3) \
		((LongWord)(Byte)(ch0) | ((LongWord)(Byte)(ch1) << 8) |   \
		((LongWord)(Byte)(ch2) << 16) | ((LongWord)(Byte)(ch3) << 24 ))
#elif TARGET_RT_BIG_ENDIAN		// Native Network order style (eg, Motorola)
	#define LE2H16(x) _swap16(x)
	#define LE2H32(x) _swap32(x)
	#define LE2H64(x) _swap64(x)
	#define H2LE16(x) _swap16(x)
	#define H2LE32(x) _swap32(x)
	#define H2LE64(x) _swap64(x)
	#define BE2H16(x) x
	#define BE2H32(x) x
	#define BE2H64(x) x
	#define H2BE16(x) x
	#define H2BE32(x) x
	#define H2BE64(x) x
	#ifndef HTONS
		#define HTONS(x) x
		#define NTOHS(x) x
		#define HTONL(x) x
		#define NTOHL(x) x
	#endif
	#define MAKE_FOURCC(ch0, ch1, ch2, ch3) \
		((LongWord)(Byte)(ch3) | ((LongWord)(Byte)(ch2) << 8) |   \
		((LongWord)(Byte)(ch1) << 16) | ((LongWord)(Byte)(ch0) << 24 ))
#endif

// Prototypes for the subroutines that do the actual work
Word _swap16(Word);
LongWord _swap24(LongWord x);
LongWord _swap32(LongWord);
LongLongWord _swap64(LongLongWord);
Byte *_swapbytes(Byte *, int);

#endif //  ByteSwap_H

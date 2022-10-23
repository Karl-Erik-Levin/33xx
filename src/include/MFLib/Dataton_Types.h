/* Dataton_Types.h - Some basic data type definitions
**
** Various genberally useful types, macros and the like.
**
** © Copyright Dataton AB 1998, All Rights Reserved
**
** Created: 98-06-01 by Mike Fahl
** Modifyed 09-09-30 by Kalle Levin to work with 3397 Transponder
**
*/

#ifndef Dataton_Types_H
#define Dataton_Types_H

#include "ConditionalMacros.h"	// For target/compiler dependent stuff
#include <String.h>				// memset for CLRSTRUCT macro

typedef unsigned char  Boolean;
typedef unsigned char  Byte;
typedef unsigned short Word;
typedef unsigned long  LongWord;
typedef void *Ptr;

#define true  1
#define false 0
#ifndef NULL
  #define	NULL 0
#endif

#if defined(TYPE_LONGLONG) && TYPE_LONGLONG
	// Compiler supports long long type (from Apple's ConditionalMacros.h, or elsewhere)
	typedef unsigned long long LongLongWord;
	#define HILONG(x)	((LongWord)((x)>>32))
	#define LOLONG(x)	((LongWord)(x))
#else
	typedef struct LongLongWord {
		unsigned long
			high,
			low;
	} LongLongWord;
	#define HILONG(x)	((x).high)
	#define LOLONG(x)	((x).low)
#endif 	// TYPE_LONGLONG

// BEWARE of duplicate side effects!
#define MIN(x,y)	(((x)<(y))?(x):(y))
#define MAX(x,y)	(((x)>(y))?(x):(y))
#define ABS(x)		(((x)<0)?-(x):(x))
#define ODD(x)		((x)&1)
#define EVEN(x)		(!ODD(x))

LongWord CalcDiff(LongWord x, LongWord y);
#define DIFF(x, y)	(CalcDiff(x, y))

// Bit twiddling macros
#define CLEAR(x,y)	(x&=~(y))
#define SET(x,y)	(x|=(y))
#define FLIP(x,y)	(x^=(y))
#define TEST(x,y)	((x)&(y))

// Clear entire structure (pass structure itself - not ptr to it - to macro)
#define CLRSTRUCT(x) memset(&x,0,sizeof(x))

#endif // Dataton_Types_H

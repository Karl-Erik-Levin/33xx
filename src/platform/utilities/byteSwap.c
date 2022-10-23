/* byteSwap.c - Byte swapping routines

Defines macros for doing byte-swapping on 16, 32 and 64 bit quantities.
The basic macros are HTONx and NTOHx, where x is S for short or L for long.
These macros swap between network order and the hardware order of the target 
runtime, as determined by the TARGET_RT_LITTLE_ENDIAN and TARGET_RT_BIG_ENDIAN
preprocessor constants. These constants are assumed to be defined by a previous
include file. Since we want to be target intependent here, I didn't want to define
any target-specific include file name, so I left this up to the caller. An error
will be printed if these macros aren't defined.

© Copyright Dataton AB 1999, All Rights Reserved

Created: 99-05-27 by Mike Fahl

*/
//---------------------------------INCLUDES---------------------------------------
#include "MFLib\Dataton_Types.h"

#include "platform\utilities\byteSwap.h"

//---------------------------------DEFINES----------------------------------------
#define carr(x) ((Byte *)&x)

//--------------------------------FUNCTIONS---------------------------------------
/*******************************************************************************
 * Function:	_swap16
 * Summary:		
 *******************************************************************************/
Word 
_swap16(Word x) 
{
    Word t;

    carr(t)[0] = carr(x)[1];
    carr(t)[1] = carr(x)[0];
    return t;
}

/*******************************************************************************
 * Function:	_swap24
 * Summary:		
 *******************************************************************************/
LongWord 
_swap24(LongWord x) 
{
    Word t;

    carr(t)[0] = carr(x)[2];
    carr(t)[1] = carr(x)[1];
    carr(t)[2] = carr(x)[0];
  
    return t;
}

/*******************************************************************************
 * Function:	_swap32
 * Summary:		
 *******************************************************************************/
LongWord 
_swap32(LongWord x) 
{
    LongWord t;
    
	#ifdef NO_LONG_CHARSWAP
	    register Byte *p = &(carr(x)[3]);
	    register Byte *q = carr(t);

	    while (p >= carr(x))
	            *q++ = *p--;
	#else
	    carr(t)[0] = carr(x)[3];
	    carr(t)[1] = carr(x)[2];
	    carr(t)[2] = carr(x)[1];
	    carr(t)[3] = carr(x)[0];
	#endif
    return t;
}

/*******************************************************************************
 * Function:	_swap64
 * Summary:		
 *******************************************************************************/
LongLongWord 
_swap64(LongLongWord x) 
{
    LongLongWord t;
    
	#ifdef NO_QUAD_CHARSWAP
	    register Byte *p = &(carr(x)[7]);
	    register Byte *q = carr(t);

	    while (p >= carr(x))
	            *q++ = *p--;
	#else
	    carr(t)[0] = carr(x)[7];
	    carr(t)[1] = carr(x)[6];
	    carr(t)[2] = carr(x)[5];
	    carr(t)[3] = carr(x)[4];
	    carr(t)[4] = carr(x)[3];
	    carr(t)[5] = carr(x)[2];
	    carr(t)[6] = carr(x)[1];
	    carr(t)[7] = carr(x)[0];
	#endif
    return t;
}
/*******************************************************************************
 * Function:	_swapbytes
 * Summary:		This is a byte number independent version of the two loop
 *				variations of _swap32() and _swap64() above. 
 *				It does in-place swap, and REQUIRES that 'num' be 0 modulus 2
 *				(i.e, an even number).
 *******************************************************************************/
Byte *
_swapbytes(Byte *x, int num) 
{
    register Byte *q = &(x[num / 2]);
    register Byte *p = (q - 1);
    register Byte r;

    while (p >= x) {
            r = *p;
            *p-- = *q;
            *q++ = r;
    }
    return x;
}

/*******************************************************************************
 * Function:	CalcDiff
 * Summary:		
 *******************************************************************************/
LongWord
CalcDiff(LongWord x, LongWord y)
{
	return x > y ? (x - y) : (y - x);
}


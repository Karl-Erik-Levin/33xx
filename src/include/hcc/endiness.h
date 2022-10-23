/****************************************************************************
 *
 *            Copyright (c) 2006-2008 by HCC Embedded
 *
 * This software is copyrighted by and is the sole property of
 * HCC.  All rights, title, ownership, or other interests
 * in the software remain the property of HCC.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of HCC.
 *
 * HCC reserves the right to modify this software without notice.
 *
 * HCC Embedded
 * Budapest 1133
 * Vaci ut 110
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef _ENDINESS_H_
#define _ENDINESS_H_

#include "HCC/compiler.h"

/* This should be defined before including this header file. */
#ifndef BIGENDIAN
#error "BIGENDIAN shall be defined with value 0 or 1"
#endif

/* This may be define in compiler.h if the MCU core and/or the compiler
   supports some more efficient method for reversing byte order. */
#ifndef BREW16
  /* Revert byte order of a 16 bit unsigned integer. */
  #define BREW16(a)   (((a>>8)&0xff) | ((a<<8)&0xff00))
#endif

/* This may be define in compiler.h if the MCU core and/or the compiler
   supports some more efficient method for reversing byte order. */
#ifndef BREW32
  /* Revert byte order of a 32 bit unsigned integer. */
  #define BREW32(a)   (((a>>24)&0xff) | ((a>>8)&0xff00) | ((a&0xff)<<24) | ((a&0xff00)<<8))
#endif

#if BIGENDIAN == 1
  #define LE16(l)  ((hcc_u16)BREW16(l))
  #define LE32(l)  ((hcc_u32)BREW32(l))
  #define BE16(l)  ((hcc_u16)(l))
  #define BE32(l)  ((hcc_u32)(l))
#else
  #define LE16(l)  ((hcc_u16)(l))
  #define LE32(l)  ((hcc_u32)(l))
  #define BE16(l)  ((hcc_u16)BREW16(l))
  #define BE32(l)  ((hcc_u32)BREW32(l))
#endif

#endif

/****************************** END OF FILE **********************************/

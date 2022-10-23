/****************************************************************************
 *
 *            Copyright (c) 2006-2009 by HCC Embedded
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
 * Váci út 110.
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef _IAR_COMPILER_H_
#define _IAR_COMPILER_H_

#ifndef __ICCARM__
  #error "This file shall be compiled with the IAR compiler."
#endif

/* Map compiler specific endiness setting to our system. */
#if __LITTLE_ENDIAN__ == 1
#define BIGENDIAN 0
#else
#define BIGENDIAN 1
#endif

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <intrinsics.h>

typedef uint8_t  hcc_u8;
typedef uint16_t hcc_u16;
typedef uint32_t hcc_u32;

typedef int8_t  hcc_s8;
typedef int16_t hcc_s16;
typedef int32_t hcc_s32;

typedef volatile hcc_u32 hcc_reg32;

#include "endiness.h"

#define _enable_interrupt __enable_interrupt
int _disable_interrupt(void);
void _restore_interrupt(int im);

#endif
/****************************** END OF FILE **********************************/

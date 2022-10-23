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
#ifndef _HCC_BL_TYPES_H_
#define _HCC_BL_TYPES_H_
/* Type definitions */
typedef unsigned char hcc_u8;
typedef unsigned short hcc_u16;
typedef unsigned int hcc_u32;

typedef volatile hcc_u8 bl_reg8;
typedef volatile hcc_u16 bl_reg16;
typedef volatile hcc_u32 bl_reg32;

typedef hcc_u32 hcc_imask;

#ifdef NDEBUG
#define HCC_ASSERT(c) (void)0
#else

#define HCC_ASSERT(c)\
do {\
  if(!(c))\
  {\
    int a=1;\
    while(a)\
      ;\
  }\
}while(0)
#endif


#endif

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
#ifndef _SAM7_REGS_H_
#define _SAM7_REGS_H_

#define BIT0    (1u<<0)
#define BIT1    (1u<<1)
#define BIT2    (1u<<2)
#define BIT3    (1u<<3)
#define BIT4    (1u<<4)
#define BIT5    (1u<<5)
#define BIT6    (1u<<6)
#define BIT7    (1u<<7)
#define BIT8    (1u<<8)
#define BIT9    (1u<<9)
#define BIT10   (1u<<10)
#define BIT11   (1u<<11)
#define BIT12   (1u<<12)
#define BIT13   (1u<<13)
#define BIT14   (1u<<14)
#define BIT15   (1u<<15)
#define BIT16   (1u<<16)
#define BIT17   (1u<<17)
#define BIT18   (1u<<18)
#define BIT19   (1u<<19)
#define BIT20   (1u<<20)
#define BIT21   (1u<<21)
#define BIT22   (1u<<22)
#define BIT23   (1u<<23)
#define BIT24   (1u<<24)
#define BIT25   (1u<<25)
#define BIT26   (1u<<26)
#define BIT27   (1u<<27)
#define BIT28   (1u<<28)
#define BIT29   (1u<<29)
#define BIT30   (1u<<30)
#define BIT31   (1u<<31)

#define UDP_ISR_WAKEUP      BIT13
#define UDP_ISR_ENDBUSRES   BIT12
#define UDP_ISR_SOFINT      BIT11
#define UDP_ISR_EXTRSM      BIT10
#define UDP_ISR_RXRSM       BIT9
#define UDP_ISR_RXSUSP      BIT8
#define UDP_ISR_EP3INT      BIT3
#define UDP_ISR_EP2INT      BIT2
#define UDP_ISR_EP1INT      BIT1
#define UDP_ISR_EP0INT      BIT0

#define UDP_IMR_WAKEUP      BIT13
#define UDP_IMR_ENDBUSRES   BIT12
#define UDP_IMR_SOFINT      BIT11
#define UDP_IMR_EXTRSM      BIT10
#define UDP_IMR_RXRSM       BIT9
#define UDP_IMR_RXSUSP      BIT8
#define UDP_IMR_EP3INT      BIT3
#define UDP_IMR_EP2INT      BIT2
#define UDP_IMR_EP1INT      BIT1
#define UDP_IMR_EP0INT      BIT0

#endif

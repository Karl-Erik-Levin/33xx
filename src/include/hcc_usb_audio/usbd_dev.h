/****************************************************************************
 *
 *            Copyright (c) 2008-2009 by HCC Embedded
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
 * Váci út 110
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef _USBD_DEV_H_
#define _USBD_DEV_H_

/* This header contains public, usb driver specific stuff. */

#include "src/lib/os/os.h"
#include "src/lib/compiler/compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 ************************ Macro definitions ***********************************
 *****************************************************************************/
/* none */

/******************************************************************************
 ************************ Type definitions ************************************
 *****************************************************************************/
/* This stucture defines hardware inicialization parameters.
   For the SAMX this means high and low priority interrupt
   priopity foc configuring the AIC. */
typedef struct {
  os_it_info_t it_info;
} usbd_hw_init_info_t;

typedef struct {
  hcc_u8 db;
  hcc_u8 pep;
  hcc_u8 flags;
} usbd_hw_ep_info_t;
/******************************************************************************
 ************************ Exported functions **********************************
 *****************************************************************************/
OS_ISR_DEF(usbd_it_handler);

#ifdef __cplusplus
}
#endif


#endif
/****************************** END OF FILE **********************************/

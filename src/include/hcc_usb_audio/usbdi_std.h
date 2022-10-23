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
#ifndef _USBDI_STD_H_
#define _USBDI_STD_H_

/*! \file
    \brief
    Driver internal declarations for the STD module.

    This file contains driver internal declarations of the standard module.
    These declarationsh shall only be used in other driver modules but not
    in class-drivers or user applications. */

#include "usbd_std.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************
 ************************ Exported functions **********************************
 *****************************************************************************/
/*! \brief Inicialize module. */
int usbd_std_init(void);
/*! \brief Start module operation. */
int usbd_std_start(usbd_config_t *cfg);
/*! \brief Stop module. */
int usbd_std_stop(void);
/*! \brief Kill module (free resources, etc.) */
int usbd_std_delete(void);

/******************************************************************************
 ************************* Global variables ***********************************
 *****************************************************************************/
/*! \brief The current driver status see USBDST_XXX values. */
extern int usbd_state;
/*! \brief Remote wakeup state. Nonzero if enabled. */
extern hcc_u8 rw_en;
/*! \brief Setup packet received event. */
extern OS_EVENT_BIT_TYPE usbd_stprx_event;

#ifdef __cplusplus
}
#endif


#endif
/****************************** END OF FILE **********************************/

/****************************************************************************
*
*            Copyright (c) 2009 by HCC Embedded
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


#ifndef _USBD_SPEAKER_
#define _USBD_SPEAKER_

#include "src/usb-device/usb-drivers/common/usbd.h"

/* Initialize class driver. */
int usbd_spk_init(void);

/* Start class driver. */
int usbd_spk_start(void);

int usbd_spk_stop(void);

int usbd_spk_delete(void);

/* USB driver interface functions. */
usbd_callback_state_t usbd_spk_ep0_event(const usbd_setup_data_t *stp
                                                , usbd_transfer_t *tr);
void usbd_spk_cdrv_cb(const usbd_ep_info_t *eps, const int ifc_ndx, const int para);

rngbuf_t *usbd_spk_get_out_stream(void);

void usbd_spk_set_out_sr(int rate);

void usbd_spk_set_synch_rate(hcc_u32 rate);

int usbd_spk_is_active(void);

#endif

/****************************** END OF FILE **********************************/

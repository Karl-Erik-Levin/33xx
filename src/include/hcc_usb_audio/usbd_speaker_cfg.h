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
 * Vaci ut 110
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef _USBD_AUDIO_CFG_H_
#define _USBD_AUDIO_CFG_H_

#include "src/lib/hcc_rngbuf/hcc_rngbuf.h"

/* Maximum sample rate multiplied by sample width. Multiple by 2 if
   stream is stereo. Divide result by 8000. Round up.
   This value must match the maximum packet size in the endpoint descriptor. */
#define USBD_SPK_MAX_P_SIZE 64

/* Number of packets the audio sample FIFO can hold. */
#define BUFFER_PER_EP  10

/* Sample rate refresh period. A */
#define SR_REFRESH_PERIOD 4

int usbd_spk_init_fifo(rngbuf_t *aud_fifo, rngbuf_t *feedb_fifo);
int usbd_spk_delete_fifo(rngbuf_t *aud_fifo, rngbuf_t *feedb_fifo);

#endif

/****************************** END OF FILE **********************************/

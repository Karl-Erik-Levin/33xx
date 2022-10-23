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

#include "usbd_speaker_cfg.h"
#include "src/lib/hcc_rngbuf/hcc_rngbuf.h"

/* Area to store audio sample data. Using hcc_u32 to make it 32 bit aligned.
   Size is rounded up. */
static hcc_u32 audio_storage[(((USBD_SPK_MAX_P_SIZE*BUFFER_PER_EP))+3)/4];

/* Sample rate feedback data. */
static hcc_u32 feedback_data;

/* Ring buffer items to manage audio_storage. */
rngbuf_item_t audio_buf_items[BUFFER_PER_EP+1];

#define INIT_ITEM(ndx)\
  audio_buf_items[ndx].data.data=strg_offset;\
  audio_buf_items[ndx].data.size=USBD_SPK_MAX_P_SIZE;\
  strg_offset+=USBD_SPK_MAX_P_SIZE;

int usbd_spk_init_fifo(rngbuf_t *aud_fifo, rngbuf_t *feedb_fifo)
{
  int buf_item_ndx=0;
  hcc_u8 *strg_offset=(hcc_u8*)audio_storage;

  /* Initialize buffer items, and add them to FIFO. */
  INIT_ITEM(buf_item_ndx);
  (void)rngbuf_init(aud_fifo, &audio_buf_items[buf_item_ndx]);

  for(buf_item_ndx++; buf_item_ndx<BUFFER_PER_EP; buf_item_ndx++)
  {
    INIT_ITEM(buf_item_ndx);
    rngbuf_add_item(aud_fifo, &audio_buf_items[buf_item_ndx]);
  }

  /* Create a single item for the feedback FIFO. */
  audio_buf_items[buf_item_ndx].data.data=&feedback_data;
  audio_buf_items[buf_item_ndx].data.size=3;
  (void)rngbuf_init(feedb_fifo, &audio_buf_items[buf_item_ndx]);
  return OS_SUCCESS;
}

int usbd_spk_delete_fifo(rngbuf_t *aud_fifo, rngbuf_t *feedb_fifo)
{
  (void)aud_fifo;
  (void)feedb_fifo;
  return OS_SUCCESS;
}

/****************************** END OF FILE **********************************/

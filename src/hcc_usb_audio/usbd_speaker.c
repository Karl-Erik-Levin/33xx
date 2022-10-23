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
 * Vaci ut 110
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#include "src/usb-device/class-drivers/audio/usbd_speaker.h"
#include "src/usb-device/class-drivers/audio/usbd_speaker_cfg.h"
#include "src/usb-device/usb-drivers/common/usbd.h"
#include "src/usb-device/usb-drivers/common/sof_timer.h"
#include "src/lib/os/os.h"
#include "src/lib/board/board.h"
#include "src/lib/target/target.h"
#include "src/lib/hcc_rngbuf/hcc_rngbuf.h"
#include "usbd_config.h"
#include <assert.h>

/* This class driver is compatible with the API version below. */
#if USBD_MAJOR != 5 || USBD_STD_MAJOR != 6
#error "Incorrect USB driver version."
#endif

/****************************************************************************
 ************************** Macro definitions *******************************
 ***************************************************************************/
/* Mandatory class specific requests. */
#define RQ_UNDEFINED 0x0
#define RQ_SET_CUR   0x1
#define RQ_SET_MIN   0x2
#define RQ_SET_MAX   0x3
#define RQ_SET_RES   0x4
#define RQ_SET_MEM   0x5

#define RQ_GET_CUR  0x81
#define RQ_GET_MIN  0x82
#define RQ_GET_MAX  0x83
#define RQ_GET_RES  0x84
#define RQ_GET_MEM  0x85

#define RQ_GET_STAT 0xff

static struct {
  hcc_u8 ctrl_ifc_ndx;
  hcc_u32 buffer[20/4];
  rngbuf_t fifo;
  rngbuf_t sync_fifo;
  usbd_ep_handle_t stream_ep;
  usbd_ep_handle_t sync_ep;
  softmr_event_handle_t my_sof_event;
  hcc_u8 stream_ep_addr;
  hcc_u32 curr_sr;
} usbd_spk_info;

static void reset_event(void)
{
  usbd_spk_info.ctrl_ifc_ndx=0xff;
  usbd_spk_info.stream_ep=USBD_INVALID_EP_HANDLE_VALUE;
  usbd_spk_info.sync_ep=USBD_INVALID_EP_HANDLE_VALUE;
  usbd_spk_info.stream_ep_addr=0xff;
  usbd_spk_info.curr_sr=0;
}

int usbd_spk_init(void)
{
  int r=OS_SUCCESS;

  r=usbd_spk_init_fifo(&usbd_spk_info.fifo, &usbd_spk_info.sync_fifo);

  if (OS_SUCCESS==r) r=usdb_register_cdrv(0x1, 0x1, 0x0, usbd_spk_cdrv_cb, 0, usbd_spk_ep0_event);
  if (OS_SUCCESS==r) r=usdb_register_cdrv(0x1, 0x2, 0x0, usbd_spk_cdrv_cb, 1, usbd_spk_ep0_event);
  if (OS_SUCCESS != r)
  {
    return r;
  }
  reset_event();
  return OS_SUCCESS;
}

int usbd_spk_start(void)
{
  reset_event();
  return(OS_SUCCESS);
}

int usbd_spk_stop(void)
{
  if (usbd_spk_info.stream_ep != USBD_INVALID_EP_HANDLE_VALUE)
  {
    usbd_stream_detach(usbd_spk_info.stream_ep);
    usbd_stream_detach(usbd_spk_info.sync_ep);
  }
  reset_event();
  return(OS_SUCCESS);
}

int usbd_spk_delete(void)
{
  usbd_spk_delete_fifo(&usbd_spk_info.fifo, &usbd_spk_info.sync_fifo);
  return(OS_SUCCESS);
}

void usbd_spk_sof_event(int param)
{
  rngbuf_item_t *item;

  (void)param;

  rngbuf_next_free(&usbd_spk_info.sync_fifo, &item);

  if (NULL != item)
  {
    ((hcc_u8*)item->data.data)[0]=usbd_spk_info.curr_sr & 0xff;
    ((hcc_u8*)item->data.data)[1]=(usbd_spk_info.curr_sr >> 8) & 0xff;
    ((hcc_u8*)item->data.data)[2]=(usbd_spk_info.curr_sr >> 16) & 0xff;
    item->data.nbytes=3;
    rngbuf_step(&usbd_spk_info.sync_fifo, item);
  }
}

usbd_callback_state_t usbd_spk_ep0_event(const usbd_setup_data_t *stp
                                                , usbd_transfer_t *tr)
{
  usbd_callback_state_t r=clbstc_error;
  hcc_u8 ifc_or_ep=(hcc_u8)stp->wIndex; /* Ep of ifc id. */
  switch(stp->bmRequestType)
  {
  /* Class specific out request to an endpoint. */
  case (USBRQT_DIR_OUT | USBRQT_TYP_CLASS | USBRQT_RCP_EP):
    /* Check if the EP index is ours. */
    if (usbd_spk_info.stream_ep_addr!=ifc_or_ep)
    {
      break;
    }

    switch(stp->bRequest)
    {
    /* Set endpoint control. */
    case RQ_SET_CUR:
      /* See which CS is addressed. */
      switch(stp->wValue)
      {
      	case 1<<8: /* Sampling frq control. */
	        tr->dir=USBDTRDIR_OUT;
          tr->buffer=(hcc_u8*)usbd_spk_info.buffer;
	        tr->length=3;
	        tr->zp_needed=0;
	        usbd_transfer_b(tr);
          usbd_spk_set_out_sr(((hcc_u8*)usbd_spk_info.buffer)[0] | (((hcc_u8*)usbd_spk_info.buffer)[1]<<8) |(((hcc_u8*)usbd_spk_info.buffer)[2]<<8));
          softmr_reset_event(usbd_spk_info.my_sof_event);
          os_int_disable();
          usbd_spk_sof_event(0);
          os_int_restore();
          r=clbstc_out;
     	  break;
      	case 0<<8: /* undefined cotrol. */
        case 2<<8: /* Pitch control. */
        default:
          break;
      }
      break;
    }
    break;
  }
  return(r);
}

void usbd_spk_cdrv_cb(const usbd_ep_info_t *eps, const int ifc_ndx, const int param)
{

  if (ifc_ndx == -1)
  {
  	reset_event();
  	return;
  }

  /* If this is a control interface. */
  if (0==param)
  {
	  usbd_spk_info.ctrl_ifc_ndx=(hcc_u8)ifc_ndx;
  }
  else if (1==param)
  {
    if (NULL==eps)
    {
      usbd_spk_info.stream_ep=
      usbd_spk_info.sync_ep=USBD_INVALID_EP_HANDLE_VALUE;
      usbd_spk_info.stream_ep_addr=0xff;
      softmr_del_event(usbd_spk_info.my_sof_event);
      usbd_spk_info.my_sof_event=SOFTMR_INVALID_EVENT;
    }
  	while(eps)
  	{
  	  if (eps->addr < 0x80)
  	  {
  	  	usbd_spk_info.stream_ep=eps->eph;
  	  	usbd_spk_info.stream_ep_addr=eps->addr;
        usbd_stream_attach(eps->eph,&usbd_spk_info.fifo);
  	  }
  	  else
  	  {
  	  	usbd_spk_info.sync_ep=eps->eph;
        /* TODO: get refresh period from descriptor! */
        usbd_stream_attach(eps->eph,&usbd_spk_info.sync_fifo);
        usbd_spk_info.my_sof_event=softmr_add_event(usbd_spk_sof_event, 0, SR_REFRESH_PERIOD);
        assert(SOFTMR_INVALID_EVENT != usbd_spk_info.my_sof_event);
  	  }
  	  eps=eps->next;
  	}
  }
}

rngbuf_t *usbd_spk_get_out_stream(void)
{
  return(&usbd_spk_info.fifo);
}

void usbd_spk_set_synch_rate(hcc_u32 rate)
{
  hcc_u32 i;
  hcc_u32 f;
  /* Convert rate to 10.10 format. */
  i=rate/1000u;
  f=rate%1000u;
  f=((f<<10)+500u)/1000u;

  usbd_spk_info.curr_sr=(i<<14) | (f<<4);
}

int usbd_spk_is_active(void)
{
  return(usbd_spk_info.stream_ep != USBD_INVALID_EP_HANDLE_VALUE);
}

/****************************** END OF FILE **********************************/

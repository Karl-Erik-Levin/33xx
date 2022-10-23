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
#include "driver/hccusb/usb.h"
#include "platform/hccusb/usb_mst.h"
#include "platform/hccusb/simple_scsi.h"
#include "hcc/target.h"

/****************************************************************************
 ************************** Macro definitions *******************************
 ***************************************************************************/
#define mst_process_command scsi_process_command
#define mst_setup_command   scsi_setup_command

#define MST_IFC_INDEX 0u
#define EP_RX_NO 1u
#define EP_TX_NO 2u

#define MST_BLOCK_SIZE    512
#define MST_BUFFER_SIZE   MST_BLOCK_SIZE*8

#define NEXT_BUF(b)   (b ? 0 : 1)

/* State after reset and transfer end. */
#define MSTST_IDLE        0x1u
#define MSTST_WAIT_CBW    0x2u
#define MSTST_DATA_TX     0x3u
#define MSTST_DATA_RX     0x4u
#define MSTST_DATA_NONE   0x5u
#define MSTST_SEND_STATUS 0x6u
#define MSTST_STATUS      0x7u

/* Mass-storage related standard request codes. */
#define RQ_MASS_STORAGE_RESET   0xffu
#define RQ_GET_MAX_LUN          0xfeu

#define DEBUG_TRACE 1

#if DEBUG_TRACE==1
  #define MK_DBGTRACE(evt) 		if (trace_mndx<200) {dbg_mtraces[trace_mndx++]=(evt);}
  #define MK_DBGTRACE_ONCE(evt) if (trace_mndx<200 && (dbg_mtraces[trace_mndx-1]!=(evt))) {dbg_mtraces[trace_mndx++]=(evt);}
//  #define MK_DBGTRACE(evt) (dbg_mtraces[trace_mndx++]=(evt))
//  #define MK_DBGTRACE_ONCE(evt) (dbg_mtraces[trace_mndx-1] != (evt) ? MK_DBGTRACE(evt) : 0)
#else
  #define MK_DBGTRACE(evt)      ((void)0)
  #define MK_DBGTRACE_ONCE(evt) ((void)0)
#endif

/****************************************************************************
 ************************** Local types *************************************
 ***************************************************************************/
typedef enum {
  evt_none = 0,
  evt_usb_cmd_reset,
  evt_usb_cmd_getnlun,
  evt_usb_reset,
  evt_usb_cmd_unknown,
  evt_tx_transfer_done,
  evt_tx_buffer_done,
  evt_rx_transfer_done,
  evt_rx_buffer_done,
  evt_fill_sbw_error,
  evt_fill_sbw_ok,
  evt_mst_reset,
  evt_idle_data_rx,
  evt_short_cbw,
  evt_bad_cbw_signature,
  evt_cbw_not_meaningfull,
  evt_scsi_cmd_error,
  evt_start_tx,
  evt_start_rx,
  evt_start_data_none,
  evt_data_none_scsi_process,
  evt_data_none_2_send_status,
  evt_data_tx_2_send_status,
  evt_new_buffer4usb,
  evt_data_rx_2_sent_status,
  evt_send_sbw,
  evt_scsi_process_error,
  evt_scsi_process_ok,
  evt_wait_next_cbw,
  evt_wait_next_cbw_imposibile,  
  evt_reset_req_reset,
  evt_state_no_host,
  evt_state_idle,
  evt_state_data_none,
  evt_state_data_tx,
  evt_state_data_rx,
  evt_state_send_status,
  evt_state_status,
  evt_usb_configured,
  evt_rx_bulk_error,
  evt_tx_bulk_error
} dbg_evt;
/****************************************************************************
 ************************** Function predefinitions. ************************
 ***************************************************************************/
enum callback_state mst_bulk_tx_event(void);
enum callback_state mst_bulk_rx_event(void);
enum callback_state mst_ctrl_event(void);

void mst_reset_event(void);
void mst_reset(void);
void wait_cbw(void);

/****************************************************************************
 ************************** Global variables ********************************
 ***************************************************************************/
struct buffers_t {
  hcc_u32 data[MST_BUFFER_SIZE/sizeof(hcc_u32)];
  hcc_u8 state;
};

datatonCB datatonApplCallback;
vendorReqCB datatonVendorCallback;

#ifndef PICKUP
static struct buffers_t buffers[2];
#else
struct buffers_t *buffers;
#endif

static hcc_u8 cur_buf;

const usb_callback_table_t usb_callback_table = {
  mst_reset_event,  /* Reset */
  0,          /* Bus error */
  0,          /* suspend */
  0,          /* wakeup */
  pup_on_off,          /* pup_on_off */
  { mst_ctrl_event, 0, 0, 0} /* Endpoint event handlers. */
};


/* CBW and SBW needs to be word aligned. */
static struct {
  union {
    command_block_wrapper_t cbw;
    hcc_u32 dummy;
  } cbw;
  union {
    status_block_wrapper_t sbw;
    hcc_u32 dummy;
  } sbw;
  hcc_u8 state;
  volatile hcc_u8 rx_event;
  volatile hcc_u8 tx_event;
  volatile hcc_u8 reset_req;
  hcc_u8 early_end;
  hcc_u8 phase_error;
} mst_info;

#if DEBUG_TRACE==1
dbg_evt dbg_mtraces[0x100];

static hcc_u8 trace_mndx=0;
#endif

/****************************************************************************
 ************************** Function definitions ****************************
 ***************************************************************************/


/*****************************************************************************
 * USB callback function. Is called by the USB driver if an USB reset event
 * occuers.
 ****************************************************************************/
void mst_reset_event(void)
{
  MK_DBGTRACE(evt_usb_reset);
  mst_info.reset_req=1;
}

/*****************************************************************************
 * USB callback function. Is called by the USB driver if a non standard
 * request is received from the HOST. This callback extends the known
 * requests by masstorage related ones.
 ****************************************************************************/
enum callback_state mst_ctrl_event(void)
{
  hcc_u8 *pdata=usb_get_rx_pptr(0);

  hcc_u8 num_of_lun;

  enum callback_state r=(STP_REQU_TYPE(pdata) & 1u<<7) ? clbst_in: clbst_out;

  /* Is this a classspecific request? */
  if ((STP_REQU_TYPE(pdata) & (0x3<<5)) == (0x1<<5))
  {
    switch(STP_REQUEST(pdata))
    {
    case RQ_MASS_STORAGE_RESET:
      if (STP_VALUE(pdata) == 0
          && STP_LENGTH(pdata) == 0
          && STP_INDEX(pdata) == MST_IFC_INDEX)
      {
        MK_DBGTRACE(evt_usb_cmd_reset);
        usb_set_halt(EP_RX_NO);
        usb_set_halt(EP_TX_NO);
        mst_info.reset_req=1;
      }
      else
      {
        r=clbst_error;
      }
      break;
    case RQ_GET_MAX_LUN:
      if (STP_VALUE(pdata) == 0
          && STP_LENGTH(pdata) == 1
          && STP_INDEX(pdata) == MST_IFC_INDEX)
      {
        MK_DBGTRACE(evt_usb_cmd_getnlun);
      num_of_lun=scsi_get_nlun()-1;
      usb_send(0, (void *) 0, (hcc_u8*)&num_of_lun, 1, 1, STP_LENGTH(pdata));
      }
      else
      {
        r=clbst_error;
      }
      break;
    default:
      r=clbst_error;
    }
  }
  /* Is this a vendor specific request? */
  else if ((STP_REQU_TYPE(pdata) & (0x3<<5)) == (0x2<<5))
  {
  	if (datatonVendorCallback != (void *) 0)
  	{
  	  datatonVendorCallback(pdata);		// Call Dataton vendor request handler
  	}									// Setup messages from Pickup charger
  }
  else
  {
    MK_DBGTRACE(evt_usb_cmd_unknown);
    r=clbst_error;
  }
  return(r);
}

/*****************************************************************************
 * USB callbac function. Will be called by the USB driver if a bulk IN
 * trensmission ends, or the driver needs more data.
 ****************************************************************************/
enum callback_state mst_bulk_tx_event(void)
{
  /* Transfer end? */
  if(!usb_ep_is_busy(EP_TX_NO))
  {
    MK_DBGTRACE(evt_tx_transfer_done);
    mst_info.tx_event=1;
    return(clbst_ok);
  }
  else if (!usb_ep_error(EP_TX_NO))
  {
    MK_DBGTRACE(evt_tx_buffer_done);
    mst_info.tx_event=2;
    /* Suspend endpoint. */
    return(clbst_not_ready);
  }
  else
  {
    MK_DBGTRACE(evt_tx_bulk_error);
    return(clbst_error);
}
}

/*****************************************************************************
 * USB callbac function. Will be called by the USB driver if a bulk OUT
 * trensmission ends, or the receive buffer is full.
 ****************************************************************************/
enum callback_state mst_bulk_rx_event(void)
{
  if(!usb_ep_is_busy(EP_RX_NO))
  {
    MK_DBGTRACE(evt_rx_transfer_done);
    mst_info.rx_event=1;
    return(clbst_ok);
  }
  else if (!usb_ep_error(EP_RX_NO))
  {
    MK_DBGTRACE(evt_rx_buffer_done);
    mst_info.rx_event=2;
    return(clbst_not_ready);
  }
  else
  {
    MK_DBGTRACE(evt_rx_bulk_error);
    return(clbst_error);
  }
}

static void fill_sbw(command_block_wrapper_t *cbw, status_block_wrapper_t *sbw)
{
  hcc_u8 dir=CBW_FLAGS(mst_info.cbw.cbw) & CBW_FLAGS_DIR ? DIR_TX : DIR_RX;

  SBW_SIGNATURE(sbw)=0x53425355ul;
  SBW_TAG(sbw)=CBW_TAG(cbw);
  if (dir == DIR_TX)
  { /* If we send data to the host */
    /* and scsi has an error, then the scsi layer has info about the amount
       of unsent data. */
    if (scsi_has_error())
    {
      SBW_DATA_RESIDUE(sbw)=scsi_get_remaining();
    }
    else
    { /* Otherwise we may have data in usb bufers, so we ask USB for the
         residue. */
      SBW_DATA_RESIDUE(sbw)=usb_get_remaining(EP_TX_NO);
    }
  }
  else
  { /* If we receive data, the scsi knows hom much data has not been written
       to the media. (Data in usb buffers is lost and needs to be retransmitted
       by the host, so we don't care about that amount.*/
    SBW_DATA_RESIDUE(sbw)=scsi_get_remaining();
  }
  /* If we have any SCSI or USB erros, then we report a check condition status
     to the host. */
  if (scsi_has_error() || usb_ep_error(EP_RX_NO) || usb_ep_error(EP_TX_NO))
  {
    MK_DBGTRACE(evt_fill_sbw_error);
    SBW_STATUS(sbw)=mst_info.phase_error ? 2 : 1;
  }
  else
  {
    MK_DBGTRACE(evt_fill_sbw_ok);
    SBW_STATUS(sbw)=0;
  }
}



void mst_reset(void)
{
  MK_DBGTRACE(evt_mst_reset);
  mst_info.reset_req=0;
  mst_info.tx_event=0;
  mst_info.rx_event=0;

  /* TODO: if any endpoint is busy, abort now! */

  /* Clear endpoint error status. */
  (void)usb_ep_error(EP_RX_NO);
  (void)usb_ep_error(EP_TX_NO);
  usb_clr_halt(EP_RX_NO);
  usb_clr_halt(EP_TX_NO);

  /* Enter idle state. */
  mst_info.state=MSTST_IDLE;
}


void wait_cbw(void)
{
  /* Enable reception of next CBW. */
  mst_info.rx_event=0;
  if (!usb_receive(EP_RX_NO, mst_bulk_rx_event, (void *)&mst_info.cbw, sizeof(command_block_wrapper_t), sizeof(command_block_wrapper_t)))
    {
    MK_DBGTRACE(evt_wait_next_cbw);
    mst_info.state=MSTST_WAIT_CBW;
    }
    else
    {
    MK_DBGTRACE(evt_wait_next_cbw_imposibile);
    }
}

void mst_init(void)
{
  scsi_init();
}

#ifdef PICKUP
hcc_u16 mst_req_bufsize_dataton(void)
{
  return 2*(sizeof(struct buffers_t)+10);
}

void mst_init_dataton(void *bufPtr, datatonCB cb)
{
  buffers = (struct buffers_t *) bufPtr;
  datatonApplCallback = cb;
  mst_init();
}

void mst_set_vendorReq(vendorReqCB cb)
{
	datatonVendorCallback = cb;
}
#endif

void mst_process(void)
{
  /* Handle usb_configuration changes. */
  if (usb_get_state() == USBST_PRECONFIGURED)
  {
    /* This means, the active configuration has been changed
       by the host. If we would support multiple configurations
       implementin different functionality in them, then the application
       had to switch functionality now. After the functionality is
       switched the change needs to be acknowledged to the driver. Before
       that no endpoint can be used except ep0. */
    /* The only configuration we have has index 1. So acknowledge this
       config. */
    MK_DBGTRACE(evt_usb_configured);
    usb_ack_config(1);
    mst_info.state=MSTST_IDLE;
  }

  if (mst_info.reset_req)
  {
    MK_DBGTRACE(evt_reset_req_reset);
    mst_reset();
  }

  if (usb_get_state() != USBST_CONFIGURED)
  {
    return;
  }

  switch(mst_info.state)
  {
  case MSTST_IDLE:
    MK_DBGTRACE_ONCE(evt_state_no_host);
    wait_cbw();
    break;
  case MSTST_WAIT_CBW:
    MK_DBGTRACE_ONCE(evt_state_idle);
    if(mst_info.rx_event)
    {
      mst_info.rx_event=0;
      MK_DBGTRACE(evt_idle_data_rx);
      /* We shall receive a CBW. So check that te received packet was a CBW. */
      /* Check transfer length.It shall be 31! */
          /* if there is a remainder, then we receiwed too few data.*/
          /* if we receiwed too much, then the USB driver will end the transfer
             and report an error. */
      if (usb_get_remaining(EP_RX_NO) != 0)
      {
        MK_DBGTRACE(evt_short_cbw);
        usb_set_halt(EP_RX_NO);
        usb_set_halt(EP_TX_NO);
        break;
      }
      if ((CBW_SIGNATURE(mst_info.cbw.cbw) == 0x43425355ul))
      {/* We got a valid CBW. */
        /* See if the CBW is meaningfull. (Note: length lun and command block will
         be checked by the command block interpreter layer. */
        if (((CBW_CB_LENGTH(mst_info.cbw.cbw) & 0xe0) == 0) && ((CBW_LUN(mst_info.cbw.cbw) & 0xf0) == 0))
        {
          hcc_u8 dir=CBW_FLAGS(mst_info.cbw.cbw) & CBW_FLAGS_DIR ? DIR_TX : DIR_RX;
          scsi_ret_val r;
          if (CBW_TRANSFER_LENGTH(mst_info.cbw.cbw) == 0)
          {
            dir=DIR_NONE;
          }

          cur_buf=0;
          mst_info.phase_error=0;
          r=mst_setup_command(CBW_LUN(mst_info.cbw.cbw), CBW_CB(mst_info.cbw.cbw), CBW_CB_LENGTH(mst_info.cbw.cbw)
                              , dir, CBW_TRANSFER_LENGTH(mst_info.cbw.cbw), (hcc_u8*)(hcc_u8*)buffers[0].data, sizeof(buffers[0].data));
          if(r != scsirv_ok)
          {
            MK_DBGTRACE(evt_scsi_cmd_error);
            if (r==scsirv_phase_err)
            {
              mst_info.phase_error=1;
            }
            /* Stall the endpoint and send an sbw after recovery. */
            mst_info.state = MSTST_SEND_STATUS;
            if (dir==DIR_RX)
            {
              usb_set_halt(EP_RX_NO);
              usb_clr_halt(EP_RX_NO);
            }
            else
            {
              usb_set_halt(EP_TX_NO);
              usb_clr_halt(EP_TX_NO);
            }
          }
          else /* Command accepted by SCSI layer. */
          {
            if (dir == DIR_TX)
            {
              /* Send first data chunk. */
              hcc_u32 r;

              MK_DBGTRACE(evt_start_tx);
              /* Get first data chunk from SCSI, and start USB send. */
              r=scsi_process();
              cur_buf=NEXT_BUF(cur_buf);
              /* Is there something to be send? */
              if (r)
              {
                hcc_u32 tlength;
                /* Calculate the current transfer length. If the scsi layer has no more
                   data, then we send what we have.
                   Otherwise we will send the number of bytes requested by the pc. */
                if (scsi_get_state() == STSCSI_IDLE)
                {
                  tlength = r;
                }
                else
                {
                  tlength = CBW_TRANSFER_LENGTH(mst_info.cbw.cbw);
                }
                /* Start USB. */
                mst_info.tx_event=0;
                mst_info.state = MSTST_DATA_TX;
                usb_send(EP_TX_NO, mst_bulk_tx_event, (hcc_u8*)buffers[0].data, r, tlength
                       , CBW_TRANSFER_LENGTH(mst_info.cbw.cbw));
                mst_info.early_end =
                  CBW_TRANSFER_LENGTH(mst_info.cbw.cbw) > tlength ? 1 : 0;
              }
              else
              { /* Report error on USB. */
                mst_info.state = MSTST_SEND_STATUS;
                usb_set_halt(EP_TX_NO);
                usb_clr_halt(EP_TX_NO);
              }
            }
            else if (dir == DIR_RX)
            {
              MK_DBGTRACE(evt_start_rx);
              mst_info.early_end = 0;
              mst_info.state = MSTST_DATA_RX;
              /* Enable reception of first packet. */
              mst_info.rx_event=0;
              usb_receive(EP_RX_NO, mst_bulk_rx_event, (hcc_u8*)buffers[0].data, sizeof(buffers[0].data)
                          , CBW_TRANSFER_LENGTH(mst_info.cbw.cbw));
            }
            else
            {
              MK_DBGTRACE(evt_start_data_none);
              mst_info.early_end = 0;
              /* If direction is none, only the status shall be
                 reported over the USB, but SCSI may need cpu time
                 to finish. So enter data_none status if needed. */
              if (scsi_get_state() != STSCSI_IDLE)
              {
                mst_info.state=MSTST_DATA_NONE;
              }
              else
              {
                mst_info.state=MSTST_SEND_STATUS;
              }
            }
          }/* else command accepted*/
        }/* Meaningfull CBW. */
        else
        {
          MK_DBGTRACE(evt_cbw_not_meaningfull);
          usb_set_halt(EP_RX_NO);
          usb_set_halt(EP_TX_NO);
        }
      }/* Valid CBW. */
      else
      {/* If CBW is invalid stall in and out pipes. */
        MK_DBGTRACE(evt_bad_cbw_signature);
        usb_set_halt(EP_RX_NO);
        usb_set_halt(EP_TX_NO);
    }
    }
    break;
  case MSTST_DATA_NONE:
    MK_DBGTRACE_ONCE(evt_state_data_none);
    if (scsi_get_state() != STSCSI_IDLE)
    {
      MK_DBGTRACE(evt_data_none_scsi_process);
      scsi_process();
    }
    else
    {
      /* Enter "send status" state. */
      MK_DBGTRACE(evt_data_none_2_send_status);
      mst_info.state = MSTST_SEND_STATUS;
    }
    break;
  case MSTST_DATA_TX:
    MK_DBGTRACE_ONCE(evt_state_data_tx);
    /* Give CPU to SCSI if it has some data to transfer. */
    if (scsi_get_state() != STSCSI_IDLE)
    {
      hcc_u32 r;
      /* Give buffer to SCSI. */
      scsi_new_buffer(0, (hcc_u8*)buffers[cur_buf].data, sizeof(buffers[cur_buf].data));
      /* Process buffer (let scsi fill buffer). */
      r=scsi_process();
      cur_buf=NEXT_BUF(cur_buf);
      if (r==0)
      { /* No data to be sent, so there is an SCSI error. */
        MK_DBGTRACE(evt_scsi_process_error);
        usb_set_halt(EP_TX_NO);
        usb_clr_halt(EP_TX_NO);
        mst_info.state = MSTST_SEND_STATUS;
        break;
      }
      MK_DBGTRACE(evt_scsi_process_ok);
    }

    /* Wait till USB finishes previous buffer. */
    while(mst_info.tx_event==0)
    {
      if (!usb_ep_is_busy(EP_TX_NO))
      {
        return;
      }
    }

    if (mst_info.tx_event == 1) /* Transmission finished. */
      {
      datatonApplCallback(STSCSI_READ_10);		// SD card read
      mst_info.tx_event=0;
      MK_DBGTRACE(evt_data_tx_2_send_status);
      /* Data transmission done, send status. */
      mst_info.state = MSTST_SEND_STATUS;
      }
    /* USB needs the next data chunk. */
    else if (mst_info.tx_event == 2)
    {
      datatonApplCallback(STSCSI_READ_10);		// SD card read
      mst_info.tx_event=0;
      /* Give next buffer to USB. (Resume suspended endpoint.) */
      MK_DBGTRACE(evt_new_buffer4usb);
      usb_resume_tx(EP_TX_NO, (hcc_u8*)buffers[NEXT_BUF(cur_buf)].data, sizeof(buffers[NEXT_BUF(cur_buf)].data));
    }
    break;
  case MSTST_DATA_RX:
    MK_DBGTRACE_ONCE(evt_state_data_rx);
    /* USB needs attention. */
    if(mst_info.rx_event)
    {
      datatonApplCallback(STSCSI_WRITE_10);		// SD card write
      /* If USB needs more buffer, give USB next free buffer. */
      if (mst_info.rx_event==2)
      {
        hcc_u8 old_buf=cur_buf;
        cur_buf=NEXT_BUF(cur_buf);
        mst_info.rx_event=0;
        usb_resume_rx(EP_RX_NO, (hcc_u8*)buffers[cur_buf].data, sizeof(buffers[cur_buf].data));
        scsi_new_buffer(0, (hcc_u8*)buffers[old_buf].data, sizeof(buffers[old_buf].data));
      }
      else if (mst_info.rx_event==1)
      {/* Transfer done. */
        mst_info.rx_event=0;
        mst_info.state = MSTST_SEND_STATUS;
        MK_DBGTRACE(evt_data_rx_2_sent_status);
        scsi_new_buffer(0, (hcc_u8*)buffers[cur_buf].data, sizeof(buffers[cur_buf].data));
      }
      /* Give CPU time to SCSI layer. */
      /* In case of an error scsi_process will return 1. */
      if(scsi_process())
      {
        MK_DBGTRACE(evt_scsi_process_error);
        if (usb_get_remaining(EP_RX_NO))
        {
          usb_set_halt(EP_RX_NO);
          usb_clr_halt(EP_RX_NO);
        }
        /* Build and send status info. */
        mst_info.state = MSTST_SEND_STATUS;
      }
      MK_DBGTRACE(evt_scsi_process_ok);
    }
    break;
  case MSTST_SEND_STATUS:
    MK_DBGTRACE_ONCE(evt_state_send_status);
    if (mst_info.early_end)
    {
      mst_info.early_end=0;
      usb_set_halt(EP_TX_NO);
      usb_clr_halt(EP_TX_NO);
    }
    else if (!usb_ep_is_halted(EP_TX_NO))
    {
    /* Fill status block, and send it. */
    fill_sbw(&mst_info.cbw.cbw, &mst_info.sbw.sbw);
    mst_info.state = MSTST_STATUS;
    mst_info.tx_event=0;

      MK_DBGTRACE(evt_send_sbw);
    usb_send(EP_TX_NO, mst_bulk_tx_event, (hcc_u8 *)&mst_info.sbw
              , sizeof(status_block_wrapper_t), sizeof(status_block_wrapper_t)
              , sizeof(status_block_wrapper_t));
    }
    break;
  case MSTST_STATUS:
    MK_DBGTRACE_ONCE(evt_state_status);
    if (mst_info.tx_event==1) /* Transmission of sbw finished. */
    {
      mst_info.tx_event=0;
      mst_info.rx_event=0;
      mst_info.state = MSTST_IDLE;
    }
    break;
  }
}

/****************************** END OF FILE **********************************/

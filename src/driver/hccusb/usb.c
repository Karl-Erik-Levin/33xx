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
#include "hcc/hcc_types.h"
#include "hcc/sam7_regs.h"
#include "atmel/AT91SAM7S256.h"
#include "driver/hccusb/usb.h"

/*****************************************************************************
 * Check configuration macros.
 *****************************************************************************/
#ifndef IT_ROUTINE_IS_ISR
#error "IT_ROUTINE_IS_ISR shall be defined with the value 0 or 1."
#endif

#ifndef NDEBUG
#define DEBUG_TRACE 0
#endif

/*****************************************************************************
 * External references.
 *****************************************************************************/
extern const usb_callback_table_t usb_callback_table;

/*****************************************************************************
 * Local types.
 *****************************************************************************/
typedef struct {
  volatile hcc_u32 tlength;
  hcc_u32 maxlength;
  hcc_u8 * volatile address;
  hcc_u8 *buffer;
  volatile usb_callback_t data_func;
  volatile hcc_u16 blength;
  hcc_u16 psize;
  volatile hcc_u8 state;
  volatile hcc_u8 flags;
  volatile hcc_u8 error;
} ep_info_t;

/*****************************************************************************
 * Macro definitions.
 *****************************************************************************/
#ifdef MIN
#undef MIN
#endif
#define MIN(a,b)  ((a) < (b) ? (a) : (b))

/* Control endpoint state machine state values. */
#define EPST_INVALID          0x00
#define EPST_PREIDLE          0x01
#define EPST_IDLE             0x02
#define EPST_DATA_TX          0x03
#define EPST_DATA_TX_DB       0x04
#define EPST_DATA_TX_START_DB 0x05
#define EPST_DATA_TX_LAST     0x06
#define EPST_DATA_TX_LAST_DB  0x07
#define EPST_DATA_RX          0x08
#define EPST_STATUS_TX        0x09
#define EPST_STATUS_RX        0x0a
#define EPST_TX_STOP          0x0b
#define EPST_TX_STOP_DB       0x0c
#define EPST_DATA_TX_WAIT_DB  0x0d
#define EPST_DATA_TX_EMPTY_DB 0x0e
#define EPST_HALTED           0x0f

/* Standard USB feature selector values. */
#define FEAT_ENDPOINT_HALT        0u
#define FEAT_DEVICE_REMOTE_WAKEUP 1u

/* Endpoint type vaues. */
#define EP_TYPE_BULK    0u
#define EP_TYPE_CONTROL 1u
#define EP_TYPE_ISO     2u
#define EP_TYPE_IT      3u
#define EP_TYPE_DISABLE 4u

/* Endpoint flag bits. */
#define EPFL_ERROR   BIT0  /* There was an error during the ongoing
                               transfer. */
#define EPFL_ZPACKET BIT1  /* After the last data packet an additional zero
                               length packet needs to be transmitted to close
                               the transfer. */
#define EPFL_START_TX BIT2 /* This flag is set before the first packet of the
                               transfer is sent out. Used by double buffered
                               endpoints. */
#define EPFL_RX_BUF1  BIT3 /* Using this bit wee keep track of buffers in case
                              of double buffered rx endpoints. */
#define NO_OF_HW_EPS 8

#if DEBUG_TRACE == 1
  enum event
  {
      ev_none = 0,
      ev_disable_ep_it,
      ev_restore_ep_it,
      ev_send_0_packet,
      ev_usb_init,
      ev_usb_stop,
      ev_set_config,
      ev_enter_default_state,
      ev_stop_ep_tx,
      ev_stop_ep_rx,
      ev_nak_ep_tx,
      ev_nak_ep_rx,
      ev_ready_ep_tx,
      ev_ready_ep_rx,
      ev_disable_ep_tx,
      ev_disable_ep_rx,
      ev_clr_dtog_tx,
      ev_clr_dtog_rx,
      ev_usb_start_tx_db,
      ev_usb_tx_db,
      ev_usb_wait_tx_db,
      ev_usb_stop_tx_db,
      ev_usb_last_tx_db,
      ev_data_tx,
      ev_data_tx_last,
      ev_usb_send,
      ev_data_rx,
      ev_usb_receive,
      ev_usb_resume_tx,
      ev_usb_resume_rx,
      ev_set_address,
      ev_get_descriptor,
      ev_get_configuration,
      ev_clr_stall,
      ev_user_ctrl_cb,
      ev_send_handshake,
      ev_it,
      ev_bus_reset,
      ev_wakeup,
      ev_suspend,
      ev_eptxit,
      ev_eprxit,
      ev_epstallit,
      ev_surious_packet,
      ev_bad_setup_length
  };

  #define MKDBG_TRACE(evnt, epndx)   if (evndx<250) {evstk[evndx].ev=(evnt), evstk[evndx++].ep=(epndx);}
#else
  #define MKDBG_TRACE(evnt, epndx)   ((void)0)
#endif
/*****************************************************************************
 * Module variables.
 *****************************************************************************/
static volatile hcc_u8 usb_current_config;
static volatile hcc_u8 usb_state;
static volatile hcc_u8 usb_state_bf_suspend;
static volatile hcc_u8 new_address;
static ep_info_t ep_info[NO_OF_HW_EPS];

#if DEBUG_TRACE == 1
struct
{
  enum event ev;
  hcc_u8 ep;
} evstk[0x100];

hcc_u8 evndx=0;
#endif

/*****************************************************************************
 * Function predefinitions.
 *****************************************************************************/
extern void usbISREntry();
void Enter_Default_State(void);
void disable_ep_rx(hcc_u8);
void disable_ep_tx(hcc_u8);
void ready_ep_rx(hcc_u8);
void ready_ep_tx(hcc_u8);
void nak_ep_tx(hcc_u8);
void nak_ep_rx(hcc_u8);
void clr_dtog_rx(hcc_u8);
void clr_dtog_tx(hcc_u8);
hcc_u8 usb_disable_ep_it(hcc_u8 ep);
void usb_restore_ep_it(hcc_u8 ep, hcc_u8 prev_state);
hcc_u8 setup_ep(hcc_u8 addr, hcc_u16 type, hcc_u8 ep, hcc_u16 psize);
hcc_u8 find_ep(hcc_u8 addr);
static enum callback_state usb_stm_ctrl0(void);
/*****************************************************************************
 * Name:
 *    usb_disable_ep_it
 * In:
 *    ep - endpoint number
 * Out:
 *    0 - it was disabled
 *    1 - it was enabled
 *
 * Description:
 *    Will disable interrupt generation of an endpoint.
 *
 * Assumptions:
 *
 *****************************************************************************/
hcc_u8 usb_disable_ep_it(hcc_u8 ep)
{
  hcc_u8 ret=*AT91C_UDP_IMR & (1<<ep) ? 1 : 0;
  *AT91C_UDP_IDR = 1<<ep;
  MKDBG_TRACE(ev_disable_ep_it, ep);
  return(ret);
}

/*****************************************************************************
 * Name:
 *    usb_restore_ep_it
 * In:
 *    ep - endpoint number
 *    prev_state - previous state of ep interrupt (0 disabled 1 enabled)
 * Out:
 *    N/A
 *
 * Description:
 *    Will disable interrupt generation of an endpoint.
 *
 * Assumptions:
 *
 *****************************************************************************/
void usb_restore_ep_it(hcc_u8 ep, hcc_u8 prev_state)
{
  if (prev_state)
  {
    *AT91C_UDP_IER = 1<<ep;
  }
  else
  {
    *AT91C_UDP_IDR = 1<<ep;
  }
  MKDBG_TRACE(ev_restore_ep_it, ep);
}

/*****************************************************************************
 * Name:
 *    send_zero_packet
 * In:
 *    N/A
 * Out:
 *    N/A
 *
 * Description:
 *    Will send a zero length data packet.
 *
 * Assumptions:
 *    ep is the index of a TX endpoint.
 *****************************************************************************/
void send_zero_packet(hcc_u8 ep)
{
  MKDBG_TRACE(ev_send_0_packet, ep);
  AT91C_UDP_CSR[ep] |= AT91C_UDP_TXPKTRDY;
  ready_ep_tx(ep);
}

/*****************************************************************************
 * Name:
 *   usb_get_remaining
 * In:
 *   ep: number of endpoint.
 * Out:
 *   The number of bytes the endpoint could not yet tranfer.
 *
 * Description:
 *   Returns te number of bytes that are left of the transfer. If
 *   usb_ep_is_busy returns false, then the transfer was aborted either
 *   by the host or by the application.
 *****************************************************************************/
hcc_u32 usb_get_remaining(hcc_u8 ep)
{
  return(ep_info[ep].tlength);
}

/*****************************************************************************
 * Name:
 *   usb_get_done
 * In:
 *   ep: number of endpoint.
 * Out:
 *   The number of bytes put to the endpoint buffer.
 *
 * Description:
 *   Returns te number of bytes that were put to the endpoint buffer.
 *****************************************************************************/
hcc_u32 usb_get_done(hcc_u8 ep)
{
  return(ep_info[ep].maxlength - ep_info[ep].tlength);
}


/*****************************************************************************
 * Name:
 *    usb_ep_is_busy
 * In:
 *   ep: number of endpoint.
 * Out:
 *   nonzero if endpoint is buys (a transfer is ongoing).
 *
 * Description:
 *
 *****************************************************************************/
hcc_u8 usb_ep_is_busy(hcc_u8 ep)
{
  return(ep_info[ep].state != EPST_IDLE
         && ep_info[ep].state != EPST_INVALID);
}

/*****************************************************************************
 * Name:
 *    usb_ep_is_halted
 * In:
 *   ep: number of endpoint.
 * Out:
 *   nonzero if endpoint is halted.
 *
 * Description:
 *
 *****************************************************************************/
hcc_u8 usb_ep_is_halted(hcc_u8 ep)
{
  return(ep_info[ep].state == EPST_HALTED
         || ((AT91C_UDP_CSR[ep] & (AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR)) != 0));
}

/*****************************************************************************
 * Name:
 *    usb_get_state
 * In:
 *   N/A
 * Out:
 *   Current USB state. See USBST_xxx in usb.h
 *
 * Description:
 *
 *****************************************************************************/
hcc_u8 usb_get_state(void)
{
  return(usb_state);
}

/*****************************************************************************
 * Name:
 *    usb_ep_error
 * In:
 *   ep: number fo endpoint
 * Out:
 *   Endpoint specific error code. (See USBEPERR_xx macro definitions in usb.h)
 *
 * Description:
 *
 *****************************************************************************/
hcc_u8 usb_ep_error(hcc_u8 ep)
{
  hcc_u8 tmp;
  hcc_u8 it;

  it=usb_disable_ep_it(ep);
  tmp=ep_info[ep].error;
  ep_info[ep].error=USBEPERR_NONE;
  usb_restore_ep_it(ep, it);
  return(tmp);
}

/*****************************************************************************
 * Name:
 *    usb_copy_to_ep
 * In:
 *   src: source address
 *   ep:  number of endpoint
 * Out:
 *   Number of bytes made ready for sending.
 *
 * Description:
 *   Copyes data to the endpoint specific buffer in the USB hardware module.
 *****************************************************************************/
static void usb_copy_to_ep(hcc_u8* src, hcc_u8 ep, hcc_u16 length)
{
  hcc_u8* end=src+length;

  while(AT91C_UDP_CSR[ep] & AT91C_UDP_TXPKTRDY)
    ;
  /* Copy data into buffers. */
  while(src != end)
  {
     /* Put data to FIFO. */
     AT91C_UDP_FDR[ep]=*src++;
  }/* for */
}

/*****************************************************************************
 * Name:
 *    usb_copy_from_ep
 * In:
 *   ep:  number of endpoint
 *   dst: source address
 * Out:
 *   Number of copied bytes.
 *
 * Description:
 *   Copyes data from the endpoint specific buffer in the USB hardware module
 *   and will clear pending interrupt in the endpoint.
 *
 *****************************************************************************/
static void usb_copy_from_ep(hcc_u8 ep, hcc_u8* dst, hcc_u16 length)
{
  int x;

  if (length ==0)
  {
    x=AT91C_UDP_FDR[ep];
  }
  else
  {
    for(x=0; x < length; x++)
    {
      *dst++=AT91C_UDP_FDR[ep];
    }
  }

  /* Make endpoint buffer ready for next reception. */
  switch(AT91C_UDP_CSR[ep] & (AT91C_UDP_RX_DATA_BK1 | AT91C_UDP_RX_DATA_BK0
                                 | AT91C_UDP_RXSETUP))
  {
  case AT91C_UDP_RX_DATA_BK0:
  usebuf0:
    ep_info[ep].flags &= ~EPFL_RX_BUF1;
    AT91C_UDP_CSR[ep] &= ~AT91C_UDP_RX_DATA_BK0;
    while (AT91C_UDP_CSR[0]  & AT91C_UDP_RX_DATA_BK0)
      ;
    break;
  case AT91C_UDP_RX_DATA_BK1:
  usebuf1:
    ep_info[ep].flags |= EPFL_RX_BUF1;
    AT91C_UDP_CSR[ep] &= ~AT91C_UDP_RX_DATA_BK1;
    while (AT91C_UDP_CSR[0]  & AT91C_UDP_RX_DATA_BK1)
      ;
    break;
  case (AT91C_UDP_RX_DATA_BK1 | AT91C_UDP_RX_DATA_BK0):
    if (ep_info[ep].flags & EPFL_RX_BUF1)
    {
      goto usebuf0;
    }
    else
    {
      goto usebuf1;
    }
  case AT91C_UDP_RXSETUP:
    /* For control endpoints we may have to clear the setup packed received flag. */
    AT91C_UDP_CSR[0] &= ~AT91C_UDP_RXSETUP;
    while (AT91C_UDP_CSR[0]  & AT91C_UDP_RXSETUP)
      ;
    break;
  default:
    HCC_ASSERT(0);
  }
}
/*****************************************************************************
 * Name:
 *    usb_init
 * In:
 *   hp - interrupt level for the high priority interrupt request
 *   lp - interrupt level for the low priority interrupt request
 * Out:
 *   0  - if all ok
 *   !0 - if failed
 *
 * Description:
 *   Initialises the usb driver. Will set the interrupt level.
 *   Note: clock sources includin the USB module clock shall be configured and
 *        stable prior the call to this function.
 *****************************************************************************/
hcc_u8 usb_init(hcc_u8 ip)
{
  int x;

  MKDBG_TRACE(ev_usb_init, 0);
  /* Disable pull-up if posibble (disconnect from USB). */
  if (usb_callback_table.pup_on_off != (void *)0)
  {
    (*usb_callback_table.pup_on_off)('\0');
  }

  /* Set the PLL USB Divider */
  AT91C_BASE_CKGR->CKGR_PLLR |= AT91C_CKGR_USBDIV_1;

  /* Enable USB clock. Note: the code assumes UDPCK has the correct 48MHz value when
     this function is called (see USBDIV in CKGR_PLLR register). */
  /* Enable USB clock in system clock enable register. */
  *AT91C_PMC_SCER=AT91C_PMC_UDP;
  /* and in peripheral clock enable register. */
  *AT91C_PMC_PCER = (1u<<AT91C_ID_UDP);


  /* Disable all USB interrupts. */
  *AT91C_UDP_IDR = -1u;

  /* Clear all pending USB interrupts. */
  *AT91C_UDP_ICR = -1u;


  /* Set default state for all endpoints. */
  for(x=0; x<sizeof(ep_info)/(sizeof(ep_info[0])); x++)
  {
    ep_info[x].state=EPST_INVALID;
    ep_info[x].error=USBEPERR_NONE;
    ep_info[x].data_func=0;
  }

  /* Put USB to default state. */
  Enter_Default_State();

  /* TODO: SMR access may need protection against interrupts !*/
  AT91C_AIC_SVR[AT91C_ID_UDP] = (hcc_u32) usbISREntry;

  /* Set interrupt priority level for the two ISB interrupts. */
  AT91C_AIC_SMR[AT91C_ID_UDP] = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | (AT91C_AIC_PRIOR & ip);

  /* Enable USB interrupts. */
  *AT91C_AIC_IECR=(1u<<AT91C_ID_UDP);


  *AT91C_UDP_IER = UDP_IMR_EXTRSM | UDP_IMR_RXRSM | UDP_IMR_RXSUSP;

  /* Enable USB data pins. */
  *AT91C_UDP_TXVC &= ~AT91C_UDP_TXVDIS;

  /* Enable pull-up if possible. */
  if (usb_callback_table.pup_on_off != (void *)0)
  {
    (*usb_callback_table.pup_on_off)(1);
  }

  return(0);
}
/*****************************************************************************
 * Name:
 *    usb_stop
 * In:
 *    N/A
 * Out:
 *    N/A
 *
 * Description:
 *   Stops operation of the USB driver. It will disable any USB related
 *   interrupts, and will put the USB module into low power mode if it is
 *   avaiable. The USB clock source may be stopped after this call.
 *****************************************************************************/
void usb_stop(void)
{
  MKDBG_TRACE(ev_usb_stop, 0);

  /* Disable pull-up if possible. */
  if (usb_callback_table.pup_on_off != (void *)0)
  {
    (*usb_callback_table.pup_on_off)(0);
  }

  /* Disable USB interrupts. */
  *AT91C_AIC_IDCR=(1u<<AT91C_ID_UDP);

  /* Stop all endpoints, and set address to 0. */
  Enter_Default_State();

  setup_ep(0, EP_TYPE_DISABLE, 0, 0);

  /* Disable embedded transceiver. */
  *AT91C_UDP_TXVC = AT91C_UDP_TXVDIS;

  usb_state=USBST_DISABLED;
}

/*****************************************************************************
 * Name:
 *    setup_ep
 * In:
 *    addr  - endpoint address. MSB shall be one if the endpoint is type IN
 *            (TX). This is the value of the endpoint address file din the
 *            endpoint descriptor.
 *    type  - endpoint type (control, bulk, interrupt, iso). This is the value
 *            of the endpoint type filed of the endpoint descriptor.
 *    ep    - number of endpoint
 *    psize - maximum packet size allowed for this endpoint.
 *    db    - nonzer of endpoint shall be double buffered. Note: only iso and
 *            bulk endpoints can be double buffered (this is hardware
 *            specific).
 * Out:
 *    0  - all ok
 *    !0 - initialisation failed
 *
 * Description:
 *    Configures the spcified endpoint.
 *****************************************************************************/
hcc_u8 setup_ep(hcc_u8 addr, hcc_u16 type, hcc_u8 ep, hcc_u16 psize)
{
  /* Disable endpoint interrupt. */
  *AT91C_UDP_IDR = 1u<<ep;

  /* Disable endpoint. */
  disable_ep_tx(ep);

  /* Reset endpoint. */
  *AT91C_UDP_RSTEP |= (1<<ep);

  ep_info[ep].flags=0;
  if (ep_info[ep].state != EPST_INVALID
      && ep_info[ep].state != EPST_IDLE
      && ep_info[ep].state != EPST_HALTED)
  {
    ep_info[ep].error=USBEPERR_HOST_ABORT;
    if (ep_info[ep].data_func)
    {
      (*ep_info[ep].data_func)();
    }
  }

  ep_info[ep].data_func=(void*)0;
  ep_info[ep].address=0;
  ep_info[ep].tlength=0;
  ep_info[ep].maxlength=0;
  ep_info[ep].blength=0;
  ep_info[ep].psize=psize;
  ep_info[ep].buffer=0;

  /* UnReset endpoint. */
  *AT91C_UDP_RSTEP &= ~(1<<ep);

  if (type == EP_TYPE_DISABLE)
  {
    ep_info[ep].state=EPST_INVALID;
    return(0);
  }

  if (type == AT91C_UDP_EPTYPE_CTRL)
  {
    ep_info[ep].state=EPST_IDLE;
  }
  else
  {
    ep_info[ep].state=EPST_PREIDLE;
  }

  ep_info[ep].buffer = GET_EP_BUFFER(usb_current_config, 0, 0, ep);
  /* Set endpoint type. */
    /* IN endpoints (except for control type) need a different value. */
  if ((type != AT91C_UDP_EPTYPE_CTRL) && (addr & BIT7))
  {
    AT91C_UDP_CSR[ep] = (type+(4<<8)) | AT91C_UDP_EPEDS;
  }
  else
  {
    AT91C_UDP_CSR[ep] = type | AT91C_UDP_EPEDS;
  }
  /* NAK non control rx endpoints. */
  if ((type != AT91C_UDP_EPTYPE_CTRL) && (!(addr & BIT7)))
  {
    nak_ep_rx(ep);
  }
  else
  {
    ready_ep_rx(ep);
  }

  return(0);
}

/*****************************************************************************
 * Name:
 *    set_config
 * In:
 *    cfg_ndx - index of the configuration to be activated. The value shall
 *              shall equal to one defined in a configuration descriptor.
 * Out:
 *    N/A
 *
 * Description:
 *    Configures the USB module according to the specifyed configuration.
 * Assumptions:
 *    the spefified configuration exists.
 *    the first interface descriptor is for the default alternate setting.
 *    interfaces must be numbered from 0 increasing continously (0,1,2,3...)
 *    configurations must be numbered from 0 increasing continously
 *****************************************************************************/
static void set_config(hcc_u8 cfg_ndx)
{
  const hcc_u16 typemap[5] = {AT91C_UDP_EPTYPE_CTRL, AT91C_UDP_EPTYPE_ISO_OUT
                        , AT91C_UDP_EPTYPE_BULK_OUT, AT91C_UDP_EPTYPE_INT_OUT
                        , EP_TYPE_DISABLE};
  hcc_u8 ep=1;

  MKDBG_TRACE(ev_set_config, cfg_ndx);

  usb_current_config=cfg_ndx;
  if (cfg_ndx != 0)
  {
    /* For all interfaces in this configuration. */
    hcc_u8 ifc=0;
    while(IS_IFC_NDX(cfg_ndx, ifc, 0))
    {
      hcc_u8 cfg_ep=0;
      while(IS_EP_NDX(cfg_ndx, ifc, 0, cfg_ep))
      {
        const hcc_u8 *epd=GET_EP_DESCRIPTOR(cfg_ndx, ifc, 0, cfg_ep);
        setup_ep(epd[2],typemap[epd[3] & 0x3], (hcc_u16)ep, epd[4] | (((hcc_u16)epd[5]) << 8));
        cfg_ep++;
        ep++;
      }
      ifc++;
    }
    usb_state = USBST_PRECONFIGURED;
  }
  else
  {
    usb_state=USBST_ADDRESSED;
    /* No endpoints to configure. The loop below will disable all except 0. */
  }

  /* Disable all unused endpoints. */
  while(ep < sizeof(ep_info)/sizeof(ep_info[0]))
  {
    setup_ep(0, EP_TYPE_DISABLE, ep++, 0);
  }

}

/*****************************************************************************
 * Name:
 *    Enter_Default_State
 * In:
 *    N/A
 * Out:
 *    N/A
 *
 * Description:
 *    Put USB to the default state. In this state only standard requests on
 *    the default pipe are answered, all other endpoints are disabled. The
 *    device will listen on the defautl address (0).
 *****************************************************************************/
void Enter_Default_State(void)
{
  MKDBG_TRACE(ev_enter_default_state, 0);
  /* Clear all pending USB interrupts. */
  *AT91C_UDP_ICR = -1u;
  *AT91C_UDP_IER = UDP_IMR_WAKEUP | UDP_IMR_RXSUSP;
  /* Disable function endpoints, exit adressed and configured state. */
  *AT91C_UDP_GLBSTATE  = 0;
  /* Set address to 0. */
  *AT91C_UDP_FADDR = (AT91C_UDP_FEN | 0);
  /* Configure endpoint 0. */
  setup_ep(0, AT91C_UDP_EPTYPE_CTRL, 0, GET_DEV_DESCRIPTOR()[7]);
  /* This will disable all endpoints except 0. */
  set_config(0);
  usb_state=USBST_DEFAULT;
}

/*****************************************************************************
 * Name:
 *    usb_stop_ep_tx
 * In:
 *    ep - endpoint number
 * Out:
 *    N/A
 * Description:
 *    Will stall a tx endpoint (endpoint will not transmit any more packages,
 *    all IN request from the host will be denyed with error handsake).
 *****************************************************************************/
void usb_stop_ep_tx(hcc_u8 ep)
{
  MKDBG_TRACE(ev_stop_ep_tx, ep);
  AT91C_UDP_CSR[ep] |= AT91C_UDP_FORCESTALL;
}

/*****************************************************************************
 * Name:
 *    usb_stop_ep_rx
 * In:
 *    ep - endpoint number
 * Out:
 *    N/A
 * Description:
 *    Will stall a rx endpoint (endpoint will not treceive any more packages,
 *    all OUT request from the host will be denyed with error handsake).
 *****************************************************************************/
static void usb_stop_ep_rx(hcc_u8 ep)
{
  MKDBG_TRACE(ev_stop_ep_rx, ep);
  AT91C_UDP_CSR[ep] |= AT91C_UDP_FORCESTALL;
}

/*****************************************************************************
 * Name:
 *    nak_ep_tx
 * In:
 *    ep - endpoint number
 * Out:
 *    N/A
 * Description:
 *    Will NAK an tx endpoint (endpoint will not transmit any more packages,
 *    ongoing transfer is paused).
 *****************************************************************************/
void nak_ep_tx(hcc_u8 ep)
{
  MKDBG_TRACE(ev_nak_ep_tx, ep);
  *AT91C_UDP_IDR = 1<<ep;
}

/*****************************************************************************
 * Name:
 *    nak_ep_rx
 * In:
 *    ep - endpoint number
 * Out:
 *    N/A
 * Description:
 *    Will NAK an rx endpoint (endpoint will not receive any more packages,
 *    ongoing transfer is paused).
 *****************************************************************************/
void nak_ep_rx(hcc_u8 ep)
{
  MKDBG_TRACE(ev_nak_ep_rx, ep);
  *AT91C_UDP_IDR = 1<<ep;
}

/*****************************************************************************
 * Name:
 *    ready_ep_tx
 * In:
 *    ep - endpoint number
 * Out:
 *    N/A
 * Description:
 *    Make tx endpoint ready for transmission.
 *****************************************************************************/
void ready_ep_tx(hcc_u8 ep)
{
  MKDBG_TRACE(ev_ready_ep_tx, ep);
  *AT91C_UDP_IER = 1<<ep;
}

/*****************************************************************************
 * Name:
 *    ready_ep_rx
 * In:
 *    ep - endpoint number
 * Out:
 *    N/A
 * Description:
 *    Make rx endpoint ready for reception.
 *****************************************************************************/
void ready_ep_rx(hcc_u8 ep)
{
  MKDBG_TRACE(ev_ready_ep_rx, ep);
  *AT91C_UDP_IER = 1<<ep;
}

/*****************************************************************************
 * Name:
 *    disable_ep_tx
 * In:
 *    ep - endpoint number
 * Out:
 *    N/A
 * Description:
 *    Disable TX endpoint. Endpoint behaves as it would not exist (it will not
 *    affect the USB and will not generate any events).
 *****************************************************************************/
void disable_ep_tx(hcc_u8 ep)
{
  MKDBG_TRACE(ev_disable_ep_tx, ep);
  /* This will disable both directions. */
  AT91C_UDP_CSR[ep] &= ~AT91C_UDP_EPEDS;
}

/*****************************************************************************
 * Name:
 *    disable_ep_rx
 * In:
 *    ep - endpoint number
 * Out:
 *    N/A
 * Description:
 *    Disable RX endpoint. Endpoint behaves as it would not exist (it will not
 *    affect the USB and will not generate any events).
 *****************************************************************************/
void disable_ep_rx(hcc_u8 ep)
{
  MKDBG_TRACE(ev_disable_ep_rx, ep);
  /* This will disable both directions. */
  AT91C_UDP_CSR[ep] &= ~AT91C_UDP_EPEDS;
}

/*****************************************************************************
 * Name:
 *    clr_dtog_rx
 * In:
 *    ep - endpoint number
 * Out:
 *    N/A
 * Description:
 *    Clears data toggle bit of an RX endpoint.
 *****************************************************************************/
void clr_dtog_rx(hcc_u8 ep)
{
  MKDBG_TRACE(ev_clr_dtog_rx, ep);
  /* Reset endpoint */
  *AT91C_UDP_RSTEP |= 1u<<ep;
  /* Wait till reset takes effect. */
  while(AT91C_UDP_CSR[ep] & AT91C_UDP_DTGLE)
    ;
  /* Remove reset signal. */
  *AT91C_UDP_RSTEP &= ~(1u<<ep);
}

/*****************************************************************************
 * Name:
 *    clr_dtog_tx
 * In:
 *    ep - endpoint number
 * Out:
 *    N/A
 * Description:
 *    Clears data toggle bit of an TX endpoint.
 *****************************************************************************/
void clr_dtog_tx(hcc_u8 ep)
{
  MKDBG_TRACE(ev_clr_dtog_tx, ep);
  /* Reset endpoint */
  *AT91C_UDP_RSTEP |= 1u<<ep;
  /* Wait till reset takes effect. */
  while(AT91C_UDP_CSR[ep] & AT91C_UDP_DTGLE)
    ;
  /* Remove reset signal. */
  *AT91C_UDP_RSTEP &= ~(1u<<ep);
}

/*****************************************************************************
 * Name:
 *    _usb_send
 * In:
 *    ep - endpoint number
 * Out:
 *    N/A
 * Description:
 *    This fucntion inmplements the basic state machine for transmit (IN)
 *    endpoints.
 *    Note: it is called from the interrupt handler routine and from "user
 *          space" too. The function needs to be reentrant!
 *****************************************************************************/
static void _usb_send(hcc_u8 ep)
{
  hcc_u16 length;
  switch (ep_info[ep].state)
  {
#if SUPPORTS_DOUBE_BUFFERS == 1
  case EPST_DATA_TX_START_DB:
    MKDBG_TRACE(ev_usb_start_tx_db, ep);
    /* Enter this state only if there is enough data in the buffer to fill the first
       two packets. */
    /* Wait till FIO becomes accessible. */
    while(AT91C_UDP_CSR[ep] & AT91C_UDP_TXPKTRDY)
      ;

    usb_copy_to_ep(ep_info[ep].address, ep, ep_info[ep].psize);
    AT91C_UDP_CSR[ep] |= AT91C_UDP_TXPKTRDY;
    /* Calculate transfer status. */
    ep_info[ep].blength -= ep_info[ep].psize;
    ep_info[ep].tlength -= ep_info[ep].psize;
    ep_info[ep].address = (hcc_u8*)ep_info[ep].address + ep_info[ep].psize;

    length=MIN(ep_info[ep].tlength, ep_info[ep].psize);
    /* Note: buffer length check is not needed, since usb_send will check
       the buffer contains enough data for the first 2 packets. */
    usb_copy_to_ep(ep_info[ep].address, ep, length);
    ep_info[ep].blength -= length;
    ep_info[ep].tlength -= length;
    ep_info[ep].address = (hcc_u8*)ep_info[ep].address + length;

    /* Is the just created packet the last one? */
    if (ep_info[ep].tlength == 0)
    {
      if ((length == ep_info[ep].psize) && (ep_info[ep].flags & EPFL_ZPACKET))
      {
        ep_info[ep].state=EPST_TX_STOP_DB;
      }
      else
      {
        ep_info[ep].state=EPST_DATA_TX_LAST_DB;
      }
    }
    else
    {
      ep_info[ep].state=EPST_DATA_TX_DB;
    }
    break;

  case EPST_DATA_TX_DB:
    MKDBG_TRACE(ev_usb_tx_db, ep);
    /* Ready buffer filled in previous call. */
    AT91C_UDP_CSR[ep] |= AT91C_UDP_TXPKTRDY;
    /* Calculate length of next packet. */
    length=MIN(ep_info[ep].tlength, ep_info[ep].psize);
    /* Driver need new buffer if there is not enough data to build the next
       packet. */
    if (ep_info[ep].blength < length)
    {
      /* Call user callback. */
      enum callback_state r=clbst_error;

      if (ep_info[ep].data_func != (void *)0)
      {
        r=(*ep_info[ep].data_func)();
      }

      if(r != clbst_not_ready && r!=clbst_ok)
      {
        /* Application error. Abort endpoint. */
        usb_stop_ep_tx(ep);
        ep_info[ep].error = USBEPERR_USER_ABORT;
        ep_info[ep].state=EPST_IDLE;
        return;
      }
      /* Currently no buffer avaiable. */
      if (r == clbst_not_ready)
      {
        ep_info[ep].state=EPST_DATA_TX_WAIT_DB;
        return;
      }
    }
    /* Fill packet buffer. */
    usb_copy_to_ep(ep_info[ep].address, ep, length);
    /* Calculate transfer status. */
    ep_info[ep].blength -= length;
    ep_info[ep].tlength -= length;
    ep_info[ep].address = (hcc_u8*)ep_info[ep].address + length;


    /* Is the just created packet the last one? */
    if (ep_info[ep].tlength == 0)
    {
      if ((length == ep_info[ep].psize) && (ep_info[ep].flags & EPFL_ZPACKET))
      {
        ep_info[ep].state=EPST_TX_STOP_DB;
      }
      else
      {
        ep_info[ep].state=EPST_DATA_TX_LAST_DB;
      }
    }
    break;
  case EPST_DATA_TX_WAIT_DB:
    MKDBG_TRACE(ev_usb_wait_tx_db, ep);
    /* Let second buffer get empty while waiting for a new buffer. */
    ep_info[ep].state=EPST_DATA_TX_EMPTY_DB;
    break;
  case EPST_TX_STOP_DB:
    MKDBG_TRACE(ev_usb_stop_tx_db, ep);
    /* Ready buffer filled in previous call. */
    AT91C_UDP_CSR[ep] |= AT91C_UDP_TXPKTRDY;
    /* Send a sort packet. */
    ep_info[ep].state=EPST_DATA_TX_LAST_DB;
    send_zero_packet(ep);
    break;
  case EPST_DATA_TX_LAST_DB:
    MKDBG_TRACE(ev_usb_last_tx_db, ep);
    /* Ready buffer filled in previous call. */
    AT91C_UDP_CSR[ep] |= AT91C_UDP_TXPKTRDY;
    ep_info[ep].state=EPST_DATA_TX_LAST;
    break;
#endif
  case EPST_DATA_TX:  /* Transmitting data. */
    MKDBG_TRACE(ev_data_tx, ep);
    /* Calculate length of next packet. */
    length=MIN(ep_info[ep].tlength, ep_info[ep].psize);

        /* Check if driver needs new a buffer. */
    if (ep_info[ep].blength < length)
    {
      /* Call user callback. */
      enum callback_state r=clbst_error;

      if (ep_info[ep].data_func != (void *)0)
      {
        r=(*ep_info[ep].data_func)();
      }

      if(r != clbst_not_ready && r!=clbst_ok)
      {
        /* Application error. Abort endpoint. */
        usb_stop_ep_tx(ep);
        ep_info[ep].error = USBEPERR_USER_ABORT;
        ep_info[ep].state=EPST_IDLE;
        return;
      }
      /* Currently no buffer avaiable. */
      if (r == clbst_not_ready)
      {
        return;
      }
    }

    usb_copy_to_ep(ep_info[ep].address, ep, length);
    /* Ready just filled buffer. */
    AT91C_UDP_CSR[ep] |= AT91C_UDP_TXPKTRDY;

        /* Calculate transfer status. */
    ep_info[ep].blength -= length;
    ep_info[ep].tlength -= length;
    ep_info[ep].address = (hcc_u8*)ep_info[ep].address + length;

    /* Is the just created packet the last one? */
    if ((ep_info[ep].tlength == 0))
    { /* If we send less data than the host expects, and the last packet is not
         a short packet, then wee need to send an extra short packet after the
         last data packet. */
      if ((ep_info[ep].flags & EPFL_ZPACKET) && (ep_info[ep].psize == length))
      {
        ep_info[ep].state=EPST_TX_STOP;
      }
      else
      {
        ep_info[ep].state=EPST_DATA_TX_LAST;
      }
    }
    break;
  case EPST_TX_STOP: /* Sending extra short packet. */
    ep_info[ep].state=EPST_DATA_TX_LAST;
    send_zero_packet(ep);
    break;
  case EPST_DATA_TX_LAST: /* Data transmission finished. */
    MKDBG_TRACE(ev_data_tx_last, ep);
    ep_info[ep].state=EPST_IDLE;
    /* Tell application transfer ended. */
    if (ep_info[ep].data_func != (void *) 0)
    {
      /* We don't care about the return value at this point, since we already
         sent the status, and the transfer is already ended. */
      (void)(*ep_info[ep].data_func)();
      /* Disable callbacks. */
      ep_info[ep].data_func = (void *)0;
    }
    break;
  }/*switch*/
}

/*****************************************************************************
 * Name:
 *    usb_send
 * In:
 *    ep          - endpoint number
 *    f           - pointer to user callback function. A callback will bemade
 *                  if:  - the buffer is empty and more data needs to be sent
 *                       - all transmission is finished
 *                       - in case of an error
 *    data        - pointer to data buffer
 *    buffer_size - size of data buffer
 *    tr_length   - number of bytes to be transferred.
 *    req_length  - the number of bytes the host wants to receive.
 *
 *    Note: since all packes transmission on USB are started by the host, it
 *          needs to know how many bytes shall be transferred during a
 *          transfer. Because of this the host will always tell the device
 *          how many bytes it can receive (req_length). On the other hand, the
 *          the device may have less data ready (tr_length).
 * Out:
 *    N/A
 * Description:
 *    Using this function an TX (IN) transfer can be enabled.
 *****************************************************************************/
hcc_u8 usb_send(hcc_u8 ep, usb_callback_t f, hcc_u8* data, hcc_u16 buffer_size, hcc_u32 tr_length, hcc_u32 req_length)
{
  hcc_u8 itst;

  MKDBG_TRACE(ev_usb_send, ep);
  /* Exit if endpoint is not idle. */
  if (ep_info[ep].state != EPST_IDLE
      || ((AT91C_UDP_CSR[ep] & (AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR)) != 0))
  {
    return(1);
  }

  ep_info[ep].maxlength=ep_info[ep].tlength=tr_length <= req_length ? tr_length : req_length;
  ep_info[ep].blength=MIN(buffer_size, ep_info[ep].tlength);
  ep_info[ep].data_func=f;
  ep_info[ep].address=data;
  ep_info[ep].flags=req_length > tr_length ?  EPFL_ZPACKET | EPFL_START_TX : EPFL_START_TX;
  ep_info[ep].error = USBEPERR_NONE;

#if SUPPORTS_DOUBE_BUFFERS == 1
  if (IS_EP_DBUFFERED(usb_current_config, 0, 0, ep))
  {
    /* Start double buffering if: buffer contains data to build minumum 2
       packets, or buffer contains data to build more than one packet, and the
       buffer contains all data to be transferred. */
    if (buffer_size > (ep_info[ep].psize<<1)
        || ((buffer_size > ep_info[ep].psize) && (buffer_size == ep_info[ep].tlength)))
    { /* Start double buffering. */
      ep_info[ep].state=EPST_DATA_TX_START_DB;
    }
    else
    { /* Sorry, double buffering can not be started. Any way two buffers should be
         used. */
      ep_info[ep].state=EPST_DATA_TX;
    }
  }
  else
#endif
  {
    if (((AT91C_UDP_CSR[ep] & AT91C_UDP_EPTYPE) == AT91C_UDP_EPTYPE_CTRL))
    {
      /* Set transfer direction for control endpoints. */
      AT91C_UDP_CSR[0] |= AT91C_UDP_DIR;
      while(!(AT91C_UDP_CSR[0] & AT91C_UDP_DIR))
        ;
    }
    /* Use single buffer. */
    ep_info[ep].state=EPST_DATA_TX;
  }

  itst=usb_disable_ep_it(ep);
  _usb_send(ep);
  usb_restore_ep_it(ep, itst);
  return(0);
}

/*****************************************************************************
 * Name:
 *    _usb_receive
 * In:
 *    ep - endpoint number
 * Out:
 *    N/A
 * Description:
 *    This fucntion inmplements the basic state machine for receive (OUT)
 *    endpoints. It will
 *        - call user callback functions if neccessary,
 *        - set endpoint specific error codes
 *        - reassemble packets into the specified buffer
 *    Note: it is called from the interrupt handler routine and from "user
 *          space" too. The function needs to be reentrant!
 *****************************************************************************/
static void _usb_receive(hcc_u8 ep)
{
  hcc_u16 length;

  switch(ep_info[ep].state)
  {
  case EPST_DATA_RX:
    MKDBG_TRACE(ev_data_rx, ep);

    length=(AT91C_UDP_CSR[ep] & AT91C_UDP_RXBYTECNT)>>16;

    /* Check if amount of received data is ok. */
    if (ep_info[ep].tlength < length)
    { /* Host sent too many data! This is a protocol error. */
      usb_stop_ep_rx(ep);
      ep_info[ep].state=EPST_IDLE;
      /* Clear endpoint buffer. */
      usb_copy_from_ep(ep, ep_info[ep].buffer, length);
      ep_info[ep].error = USBEPERR_TO_MANY_DATA;
      if (ep_info[ep].data_func != (void *)0)
      {
        (void)(*ep_info[ep].data_func)();
      }

      return;
    }

        /* Do we need to call the user callback function? (Do ween need mor buffer
       space?) */
    if (ep_info[ep].blength < length)
    {/* If yes, */
      /* call it... */
      enum callback_state r=clbst_error;

      /* We need to disable any further interrupts to this endpoint to
       avoid having reentrancy trouble when called from
       usb_resume_tx(). */
      nak_ep_rx(ep);

      if (ep_info[ep].data_func != (void *)0)
      {
        r=(*ep_info[ep].data_func)();
      }

      if(r != clbst_not_ready && r!=clbst_ok)
      {
        /* We have an error, abort endpoint. */
        usb_stop_ep_rx(ep);
        ep_info[ep].state=EPST_IDLE;
        /* Clear endpoint buffer. */
        usb_copy_from_ep(ep, ep_info[ep].buffer, length);
        ep_info[ep].error = USBEPERR_USER_ABORT;
        return;
      }

      if (r == clbst_not_ready)
      {
        return;
      }
    }

    usb_copy_from_ep(ep, ep_info[ep].address, length);

    ep_info[ep].blength -= length;
    ep_info[ep].tlength -= length;
    ep_info[ep].address = ((hcc_u8 *)ep_info[ep].address) + length;

    /* Was this the last data packet? */
    if ((ep_info[ep].tlength == 0) || ep_info[ep].psize != length)
    {
      /* Disable further reception of packets. */
      nak_ep_rx(ep);

      ep_info[ep].state=EPST_IDLE;
      /* Control endpoints will execute the callback after the status stage. */
      if (((AT91C_UDP_CSR[ep] & AT91C_UDP_EPTYPE) != AT91C_UDP_EPTYPE_CTRL))
      {
        /* Tell application transfer ended. */
        if (ep_info[ep].data_func != (void *) 0)
        {
          /* We don't care about the return valus since the transfer is already
             finished and we can do nothing in case of an error. */
          (void)(*ep_info[ep].data_func)();
          /* Disable callbacks. */
          ep_info[ep].data_func = (void *)0;
        }
      }
    }

    break;
  default:
//    HCC_ASSERT(0);
    /* Silently dropp unexpected packets. */
    usb_copy_from_ep(ep, 0, 0);
  }
}

/*****************************************************************************
 * Name:
 *    usb_receive
 * In:
 *    ep          - endpoint number
 *    f           - pointer to user callback function. A callback will bemade
 *                  if:  - the buffer is empty and more data needs to be sent
 *                       - all transmission is finished
 *                       - in case of an error
 *    data        - pointer to data buffer
 *    buffer_size - size of data buffer
 *    tr_length   - number of bytes to be transferred. (This shal be the same
 *                  amount that the host wants to send).
 *
 * Out:
 *    N/A
 * Description:
 *    Using this function an RX (OUT) transfer can be enabled.
 *****************************************************************************/
hcc_u8 usb_receive(hcc_u8 ep, usb_callback_t f, hcc_u8* data, hcc_u16 buffer_size, hcc_u32 tr_length)
{

  MKDBG_TRACE(ev_usb_receive, ep);
  /* Exit if the endpoint is not idle. */
  if (ep_info[ep].state != EPST_IDLE
      || ((AT91C_UDP_CSR[ep] & (AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR)) != 0))
  {
    return(1);
  }

  ep_info[ep].maxlength=ep_info[ep].tlength=tr_length;
  ep_info[ep].blength=buffer_size;
  ep_info[ep].data_func=f;
  ep_info[ep].address=data;
  ep_info[ep].state=EPST_DATA_RX;
  ep_info[ep].error=USBEPERR_NONE;
  ready_ep_rx(ep);
  return(0);
}

/*****************************************************************************
 * Name:
 *    usb_set_halt
 * In:
 *    ep - endpoint number
 * Out:
 *    N/A
 *
 * Description:
 *    Set halt feature on an endpoint.
 *
 * Assumptions:
 *
 *****************************************************************************/
void usb_set_halt(hcc_u8 ep)
{
  hcc_u16 ep_type=(AT91C_UDP_CSR[ep] & AT91C_UDP_EPTYPE);

  if (ep_type == AT91C_UDP_EPTYPE_BULK_OUT
      || ep_type == AT91C_UDP_EPTYPE_BULK_IN
      || ep_type == AT91C_UDP_EPTYPE_INT_OUT
      || ep_type == AT91C_UDP_EPTYPE_INT_IN)
  {
    hcc_u8 itst=usb_disable_ep_it(ep);
    ep_info[ep].state=EPST_HALTED;

    usb_stop_ep_tx(ep);
    usb_restore_ep_it(ep, itst);
  }
}

/*****************************************************************************
 * Name:
 *    usb_clr_halt
 * In:
 *    ep - endpoint index
 * Out:
 *    N/A
 *
 * Description:
 *    Enable host to clear stall on an endpoint and thus to get it out of
 *    halted state.
 *
 * Assumptions:
 *
 *****************************************************************************/
void usb_clr_halt(hcc_u8 ep)
{
  if (ep_info[ep].state==EPST_HALTED)
  {
    ep_info[ep].state=EPST_IDLE;
  }
}

/*****************************************************************************
 * Name:
 *    usb_resume_tx
 * In:
 *    ep          - endpoint number
 *    data        - pointer to data buffer
 *    buffer_size - size of data buffer
 *
 * Out:
 *    N/A
 * Description:
 *    Using this function the user may give additional buffer to the USB
 *    driver for processing after a callback function returned clbst_not_ready
 *    and thus suspended the endpoint.
 *****************************************************************************/
void usb_resume_tx(hcc_u8 ep, hcc_u8* data, hcc_u16 buffer_size)
{
  hcc_u8 itst;

  MKDBG_TRACE(ev_usb_resume_tx, ep);
#if SUPPORTS_DOUBE_BUFFERS == 1
  /* Wait till both buffers are empty. */
  while (ep_info[ep].state != EPST_DATA_TX_EMPTY_DB)
    ;
#endif
  itst=usb_disable_ep_it(ep);

  ep_info[ep].blength=MIN(buffer_size, ep_info[ep].tlength);
  ep_info[ep].address=data;
#if SUPPORTS_DOUBE_BUFFERS == 1
  /* Start double buffering if: buffer contains data to build minumum 2
     packets, or buffer contains data to build more than one packet, and the
     buffer contains all data to be transferred. */
  if (buffer_size > (ep_info[ep].psize<<1)
      || ((buffer_size > ep_info[ep].psize) && (buffer_size == ep_info[ep].tlength)))
  { /* Start double buffering. */
    ep_info[ep].state=EPST_DATA_TX_START_DB;
  }
  else
  { /* Sorry, double buffering can not be started. Any way two buffers should be
       used. */
    ep_info[ep].state=EPST_DATA_TX;
  }
#endif
  _usb_send(ep);
  usb_restore_ep_it(ep, itst);
}

/*****************************************************************************
 * Name:
 *    usb_resume_rx
 * In:
 *    ep          - endpoint number
 *    data        - pointer to data buffer
 *    buffer_size - size of data buffer
 *
 * Out:
 *    N/A
 * Description:
 *    Using this function the user may give additional buffer to the USB
 *    driver for processing after a callback function returned clbst_not_ready
 *    and thus suspended the endpoint.
 *****************************************************************************/
void usb_resume_rx(hcc_u8 ep, hcc_u8* data, hcc_u16 buffer_size)
{
  MKDBG_TRACE(ev_usb_resume_tx, ep);

  (void)usb_disable_ep_it(ep);
  ep_info[ep].blength=buffer_size;
  ep_info[ep].address=data;
  /* At this point interrupts are disabled, and we have at least one
     unprocessed packet in the packet memory.
     The only safe way to continue is to re-enable interrupts, and
     let the pending packet to be procerssed by the ISR as usual. */
  ready_ep_rx(ep);
}

/*****************************************************************************
 * Name:
 *    cb_set_address
 * In:
 *    N/A
 *
 * Out:
 *    N/A
 *
 * Description:
 *    This callback is used by the state machine that handles the standard
 *    requests on the default pipe to set the device address after the
 *    status stage of the "set address" request.
 *****************************************************************************/
static enum callback_state cb_set_address()
{
  if (new_address != 0)
  {
    *AT91C_UDP_GLBSTATE  = AT91C_UDP_FADDEN;
    *AT91C_UDP_FADDR = (AT91C_UDP_FEN | new_address);
    usb_state=USBST_ADDRESSED;
  }
  else
  {
    Enter_Default_State();
  }
  new_address = 0;
  return(clbst_ok);
}

/*****************************************************************************
 * Name:
 *   usb_get_rx_pptr
 * In:
 *   ep: number of endpoint.
 * Out:
 *   Pointer to packet mtmory.
 *
 * Description:
 *****************************************************************************/
hcc_u8 *usb_get_rx_pptr(hcc_u8 ep)
{
  return(ep_info[ep].buffer);
}

/*****************************************************************************
 * Name:
 *    get_rx_plength
 * In:
 *    ep - number of endpoint
 *
 * Out:
 *    packet length
 *
 * Description:
 *    Returns the length of the next unprocssed rx packet received by an
 *    endpoint.
 *****************************************************************************/
hcc_u16 usb_get_rx_plength(hcc_u8 ep)
{
  return((AT91C_UDP_CSR[ep] & AT91C_UDP_RXBYTECNT)>>16);
}

/*****************************************************************************
 * Name:
 *    find_ep
 * In:
 *    addr - address of endpoint (e.g 0x01, 0x81)
 * Out:
 *    success: ep index
 *    failure: 0xff
 *
 * Description:
 *    Will decode and handle setup packets on the default endpoint.
 * Assumptions:
 *    Is only called if a setup packet is received.
 *****************************************************************************/
hcc_u8 find_ep(hcc_u8 addr)
{
  hcc_u8 ep;

  for(ep=0; ep < NO_OF_HW_EPS; ep++)
  {
    if (ep == (addr & 0xf))
    {
      return(ep);
    }
  }
  return(0xff);
}

/*****************************************************************************
 * Name:
 *    usb_stm_ctrl0
 * In:
 *    N/A
 * Out:
 *    status of callback execution
 *
 * Description:
 *    Will decode and handle setup packets on the default endpoint.
 * Assumptions:
 *    Is only called if a setup packet is received.
 *****************************************************************************/
static enum callback_state usb_stm_ctrl0(void)
{
  hcc_u8 *pdata=ep_info[0].buffer;

  /* The return value shall reflect the direction of the transfer. */
  enum callback_state r=(STP_REQU_TYPE(pdata) & BIT7) ? clbst_in: clbst_out;

  switch (STP_REQU_TYPE(pdata) & 0x7f)
  {
  case 0: /* Standard request for the device. */
    /* Determine what request this is. */
    switch (STP_REQUEST(pdata))
    {
    case USBRQ_SET_ADDRESS:
      MKDBG_TRACE(ev_set_address, new_address);
      new_address=(STP_VALUE(pdata) & ((1u << 7)-1u));
      ep_info[0].data_func=cb_set_address;
      break;
    case USBRQ_GET_DESCRIPTOR:
      MKDBG_TRACE(ev_get_descriptor, STP_VALUE(pdata)>>8);
      switch(STP_VALUE(pdata) & 0xff00u)
      {
      case STDD_DEVICE << 8:
        usb_send(0, (void *) 0, (void*)GET_DEV_DESCRIPTOR(), GET_DEV_DESCRIPTOR()[0]
                 , GET_DEV_DESCRIPTOR()[0], STP_LENGTH(pdata));
        break;
      case STDD_CONFIG << 8:
        /* Do we have a CFG descriptor with the requested index? */
        {
          hcc_u8 cfg=STP_VALUE(pdata) & 0xffu;
          /* For index 0 we return the first config descriptor. */
          if (cfg == 0)
          {
            cfg++;
          }
          if (IS_CFGD_INDEX(cfg))
          {
            const hcc_u8 *cd;
            cd=GET_CFG_DESCRIPTOR(cfg);
            usb_send(0, (void *) 0, (void*)cd, *(hcc_u16 *)(&cd[2])
                     , *(hcc_u16 *)(&cd[2]), STP_LENGTH(pdata));
            break;
          }

        }
        /* No such descriptor. */
        r=clbst_error;
        break;
      case STDD_STRING << 8:
        /* See if te required descriptor exists. */
        if (IS_STR_INDEX(STP_VALUE(pdata) & 0xffu))
        {
          usb_send(0, (void *) 0, (void *)GET_STR_DESCRIPTOR(STP_VALUE(pdata) & 0xffu)
                   , *(hcc_u8*)GET_STR_DESCRIPTOR(STP_VALUE(pdata) & 0xffu)
                   , *(hcc_u8*)GET_STR_DESCRIPTOR(STP_VALUE(pdata) & 0xffu)
                   , STP_LENGTH(pdata));
          break;
        }
        /* No such string descriptor. */
        r=clbst_error;
        break;

      default:
        /* Call user callback if avaiable. */
        goto call_usercb;
      }
      break;
    case USBRQ_GET_CONFIGURATION:
      MKDBG_TRACE(ev_get_configuration, 0);
      usb_send(0, (void *) 0, (void *)&usb_current_config
                 , 1
                 , 1
                 , STP_LENGTH(pdata));
      break;
    case USBRQ_SET_CONFIGURATION:
      MKDBG_TRACE(ev_set_config, STP_VALUE(pdata));
      if (STP_VALUE(pdata) == 0)
      {
        set_config(0);
        break;
      }
      else if (IS_CFGD_INDEX(STP_VALUE(pdata)))
      {
        set_config(STP_VALUE(pdata));
        break;
      }
      r=clbst_error;
      break;
    case USBRQ_GET_STATUS: /* Return device status. */
      if (STP_VALUE(pdata) == 0 && STP_LENGTH(pdata) == 2)
      {
        hcc_u16 st=0x1; /* no wakeup, self powered */
        usb_send(0, (void *) 0, (void *)&st
                 , 2
                 , 2
                 , 2);
      }
      else
      {
        r=clbst_error;
      }
      break;
    case USBRQ_SYNCH_FRAME:
    case USBRQ_SET_FEATURE:
    case USBRQ_SET_DESCRIPTOR:
    default: /* Unknown or not implemented request. */
      /* Call user callback if avaiable. */
      goto call_usercb;
    }
    break;
  case 1: /* Standard request for an interface. */
    switch(STP_REQUEST(pdata))
    {
    case USBRQ_GET_STATUS: /* Return interface status. */
      if (STP_VALUE(pdata) == 0 && STP_LENGTH(pdata) == 2)
      {

        hcc_u16 st=0;
        usb_send(0, (void *) 0, (void *)&st
                 , 2
                 , 2
                 , 2);
      }
      else
      {
        r=clbst_error;
      }
      break;
    default:
      /* Call user callback if avaiable. */
      goto call_usercb;
    }
  case 2: /* Standard request for an endpoint. */
    switch(STP_REQUEST(pdata))
    {
    case USBRQ_CLEAR_FEATURE:
      switch(STP_VALUE(pdata))
      {
      case FEAT_ENDPOINT_HALT:
        {
          /* Find the endpoint with the address specified in setup packet. */
          hcc_u8 ep=find_ep(STP_INDEX(pdata));
          if (ep == 0xff)
          {
            r=clbst_error;
            break;
          }

          if (ep_info[ep].state == EPST_HALTED)
          {
            break;
          }

          /* Remove stall and clear the data toggle bit. */
          clr_dtog_tx(ep);
          AT91C_UDP_CSR[ep] &= ~AT91C_UDP_FORCESTALL;
        }
        break;
      }
      break;
    case USBRQ_GET_STATUS:
      if (STP_VALUE(pdata) == 0 && STP_LENGTH(pdata) == 2)
      {
        hcc_u16 tmp=find_ep(STP_INDEX(pdata));
        if (tmp == 0xff)
        {
          r=clbst_error;
          break;
        }
        tmp = ((AT91C_UDP_CSR[tmp] & (AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR)) != 0) ? 1 : 0;
        usb_send(0, (void *) 0, (void *)&tmp
               , 2
               , 2
               , 2);
      }
      else
      {
        r=clbst_error;
      }
      break;

    case USBRQ_SET_FEATURE:
      switch(STP_VALUE(pdata))
      {
      case FEAT_ENDPOINT_HALT:
        {
          /* Find the endpoint with the address specified in setup packet */
          hcc_u8 ep=find_ep(STP_INDEX(pdata));
          if (ep == 0xff)
          {
            r=clbst_error;
            break;
          }

          if (STP_INDEX(pdata) & 0x80)
          {
            usb_stop_ep_tx(ep);
          }
          else
          {
            usb_stop_ep_rx(ep);
          }
        }
        break;
      }
      break;
    default:
      /* Call user callback if avaiable. */
      goto call_usercb;
    }
    break;
  default:
  call_usercb:
    MKDBG_TRACE(ev_user_ctrl_cb, 0);
    r=clbst_error;
    if (usb_callback_table.ep_callback[0] != (void *)0)
    {
      /* Call it... */
      r=(*usb_callback_table.ep_callback[0])();
    }
  }
  return(r);
}

/*****************************************************************************
 * Name:
 *    send_handshake
 * In:
 *    ep: endpoint number
 * Out:
 *    N/A
 *
 * Description:
 *    Send handshake after control out transfer.
 *
 *****************************************************************************/
void send_handshake(hcc_u8 ep)
{
  MKDBG_TRACE(ev_send_handshake, ep);
  AT91C_UDP_CSR[ep] |= AT91C_UDP_TXPKTRDY;
}

/*****************************************************************************
 * Name:
 *    usb_get_frame_ctr
 * In:
 *    N/A
 * Out:
 *    16 buit frame counter value
 *
 * Description:
 *    Return the value of the frame counter. This valus is increased at each
 *    SOF event seen on the USB. (Each mS.)
 *
 *****************************************************************************/
hcc_u16 usb_get_frame_ctr(void)
{
  return(*AT91C_UDP_NUM & ((1<<11)-1));
}

/*****************************************************************************
 * Name:
 *   usg_get_configuration
 * In:
 *   n/a
 * Out:
 *   Index of the current active configuration.
 *
 * Description:
 *   Return the index of the current active configuration.
 *****************************************************************************/
hcc_u8 usg_get_configuration(void)
{
  return(usb_current_config);
}

/*****************************************************************************
 * Name:
 *   usb_ack_config
 * In:
 *  cfg: index of configuration activated by the application.
 *
 * Out:
 *   0: ok
 *   1: error
 *
 * Description:
 *   The application shall acknowledge the configuration change done
 *   by the host. Withouth this no endpoint can be used (except ep0).
 *****************************************************************************/
hcc_u8 usb_ack_config(hcc_u8 cfg)
{
  if (usb_state != USBST_PRECONFIGURED)
  {
    return(1);
  }

  if (usb_current_config==cfg)
  {
    int ep;
    for (ep=0; ep< NO_OF_HW_EPS; ep++)
    {
      if (ep_info[ep].state==EPST_PREIDLE)
      {
        ep_info[ep].state=EPST_IDLE;
      }
    }
    usb_state=USBST_CONFIGURED;
  }
  return(0);
}

/*****************************************************************************
 * Name:
 *    usb_it_handler
 * In:
 *    N/A
 * Out:
 *    N/A
 *
 * Description:
 *    Low priority interrupt handler.
 *
 * Assumptions:
 *
 *****************************************************************************/
#if IT_ROUTINE_IS_ISR == 1
ISR_DEF(usb_it_handler)
#else
void usb_it_handler(void)
#endif
{
  hcc_u16 istr;
#if (AIC_IN_PROTECTED_MODE == 1) && (IT_ROUTINE_IS_ISR == 1)
  *AT91C_AIC_IVR=0;
#endif
  /* Save irq USB status. Note: ISR bits are
     not directly routed to the AIC. Thus
     ISR must be masked with IMR to avoid
     process masked interrupts if another
     interrupt source strikes. */
  istr = *AT91C_UDP_ISR;
  istr &= *AT91C_UDP_IMR;

  MKDBG_TRACE(ev_it, istr);

  if (istr & (UDP_ISR_ENDBUSRES | UDP_ISR_WAKEUP
              | UDP_ISR_RXSUSP | UDP_ISR_SOFINT
              | UDP_ISR_RXRSM | UDP_ISR_EXTRSM))
  {
    if (istr & UDP_ISR_ENDBUSRES)
    {
      /* Clear IT flag. */
      *AT91C_UDP_ICR = UDP_ISR_ENDBUSRES;

      MKDBG_TRACE(ev_bus_reset, 0);

      /* Enter default state. */
      Enter_Default_State();
      if (usb_callback_table.reset !=(void *)0)
      {
        (*usb_callback_table.reset)();
      }
      goto isr_end;
    }

    if (istr & UDP_ISR_WAKEUP)
    {
      *AT91C_UDP_ICR = UDP_ISR_WAKEUP;

      MKDBG_TRACE(ev_wakeup, 0);

      usb_state=usb_state_bf_suspend;

      if (usb_callback_table.wakeup != (void *) 0)
      {
        (*usb_callback_table.wakeup)();
      }
    }

    if (istr & UDP_ISR_RXSUSP)
    {
      *AT91C_UDP_ICR = UDP_ISR_RXSUSP;

      MKDBG_TRACE(ev_suspend, 0);

      usb_state_bf_suspend=USBST_SUSPENDED;
      usb_state=USBST_SUSPENDED;

      if (usb_callback_table.suspend != (void *)0)
      {
        (*usb_callback_table.suspend)();
      }
    }

    if (istr & UDP_ISR_SOFINT)
    {
      *AT91C_UDP_ICR = UDP_ISR_SOFINT;
      /* empty */
    }

    if (istr & (UDP_ISR_RXRSM | UDP_ISR_EXTRSM))
    {
      *AT91C_UDP_ICR = (UDP_ISR_RXRSM | UDP_ISR_EXTRSM);
    }
  }

  if (istr & (UDP_ISR_EP0INT | UDP_ISR_EP1INT | UDP_ISR_EP2INT | UDP_ISR_EP3INT))
  {
    hcc_u8 ep;
    hcc_u8 is_rx;

    for(ep=0; ep<4;ep++)
    {
      if (istr & (1u<<ep))
      {
        break;
      }
    }

    if (AT91C_UDP_CSR[ep] & AT91C_UDP_ISOERROR)
    {
      AT91C_UDP_CSR[ep] &= ~AT91C_UDP_ISOERROR;
      if (! (AT91C_UDP_CSR[ep] & (AT91C_UDP_RX_DATA_BK0
           | AT91C_UDP_RX_DATA_BK1 | AT91C_UDP_RXSETUP
           | AT91C_UDP_TXCOMP)))
      {
        MKDBG_TRACE(ev_epstallit, ep);
        /* Clear interrupt request. */
        *AT91C_UDP_ICR = 1u<<ep;
        goto isr_end;
      }
    }

    if (AT91C_UDP_CSR[ep] & (AT91C_UDP_RX_DATA_BK0
                   | AT91C_UDP_RX_DATA_BK1 | AT91C_UDP_RXSETUP))
    {/* This is an RX packet.*/
      is_rx=1;
      MKDBG_TRACE(ev_eprxit, ep);
    }
    else
    {
      MKDBG_TRACE(ev_eptxit, ep);
      is_rx=0;
      AT91C_UDP_CSR[ep] &= ~AT91C_UDP_TXCOMP;
    }

    /* is this a control endpoint? */
    if ((AT91C_UDP_CSR[ep] & AT91C_UDP_EPTYPE) == AT91C_UDP_EPTYPE_CTRL)
    {/* then use a special state machine. */
      switch(ep_info[ep].state)
      {
      case EPST_IDLE:
      idle:
        if (AT91C_UDP_CSR[ep] & AT91C_UDP_RXSETUP)
        {
          enum callback_state r=clbst_error;

          hcc_u16 length=(AT91C_UDP_CSR[ep] & AT91C_UDP_RXBYTECNT)>>16;
          /* Reset the endpoint state. */
             /* remove any callback */
          ep_info[ep].data_func = (void*)0;
             /* clear flags*/
          ep_info[ep].flags&=EPFL_RX_BUF1;
             /* clear user error indicator */
          ep_info[ep].error=USBEPERR_NONE;

          /* Copy setup packet to memory buffer. */
          usb_copy_from_ep(0, ep_info[0].buffer, length);
          /* A setup packet must be 8 bytes long. */
          if (length !=8)
          { /* Serious error! */
            MKDBG_TRACE(ev_bad_setup_length, ep);
            usb_stop_ep_rx(0);
            usb_stop_ep_tx(0);
            break;
          }
          else
          {
            /* Call packet processing function. */
            if (ep == 0)
            {
              r=usb_stm_ctrl0();
            }
            else
            {
              if (usb_callback_table.ep_callback[ep] != (void *)0)
              {
                r=(*usb_callback_table.ep_callback[ep])();
              }
            }
          }

          switch(r)
          {
            case clbst_error:
              usb_stop_ep_tx(ep);
              usb_stop_ep_rx(ep);
              ep_info[ep].state = EPST_IDLE;
              ep_info[ep].error = USBEPERR_USER_ABORT;
              break;
            case clbst_not_ready:
              nak_ep_rx(ep);
              break;
            case clbst_in:
              ready_ep_rx(ep);
              break;
            case clbst_out:
              ready_ep_rx(ep);
              if (ep_info[ep].state == EPST_IDLE)
              {
                goto send_status;
              }
              break;
            default:
              break;
          }
        }
        else
        {/* This is not a setup packet, and we encountered a protocol error.
            Possible reasons:
               -spurious package on the bus (not our fault)
               -we ended the transfer before the host.
                  - host and device transfer length was not the same
                  - driver error miscounted packages
          */
          /* Stall endpoint to make error visible to the host. */
          ep_info[ep].error = USBEPERR_TO_MANY_DATA;
          /* This is needed to clear the interrupt. */
          usb_copy_from_ep(ep, 0, 0);

          usb_stop_ep_rx(ep);
          usb_stop_ep_tx(ep);
        }
        break;
      case EPST_DATA_TX:
        /* If there is an RX interrupt pending , stop transmission. */
        /* Clear CTRTX and CTRTX; set TX state to NAK; set RX state to VALID. */
        if (is_rx)
        {
          /* Stop sending more packets. Note: there is a race condition. If
             the host asks us for more packets before the next line takes effect,
             then we will send out an unwanted packet. In this case, the host
             may get confused since that TX packet is not an answer to the last
             RX packet. */
//          nak_ep_tx(ep);
/* TODO: reset endpoint? */
          ep_info[ep].state=EPST_IDLE;
          ep_info[ep].error=USBEPERR_HOST_ABORT;
          /* Inform application about transfer end. */
          if (ep_info[ep].data_func != (void *)0)
          {
            (void)(*ep_info[ep].data_func)();
            ep_info[ep].data_func=0;
          }
          /* Go to EPST_IDLE to do setup packet processing. */
          goto idle;
        }
        _usb_send(ep);
        break;
      case EPST_TX_STOP:
        _usb_send(ep);
        break;
      case EPST_DATA_TX_LAST:
        _usb_send(ep);
        ep_info[ep].state=EPST_STATUS_TX;
        ready_ep_rx(ep);
        break;
      case EPST_STATUS_TX:
        /* In the IN transfer status stage we received a 0 byte long DATA 1 packet. */
        ep_info[ep].state=EPST_IDLE;
        /* We could check here if the received handshake packet has the correct length
           but we could not do much if not. So let's skipp this check.
        if(((AT91C_UDP_CSR[ep] & AT91C_UDP_RXBYTECNT)>>16) != 0)
        {
        }*/
        usb_copy_from_ep(0, 0, 0);
        break;
      case EPST_DATA_RX:
        _usb_receive(ep);
        /* Was this the last packet to receive? */
        send_status:
        if(ep_info[ep].state == EPST_IDLE)
        {
          /* If there was an error, stall the status stage. */
          if (ep_info[ep].flags & EPFL_ERROR)
          {
            usb_stop_ep_tx(ep);
          }
          else
          { /* We shall not be here if tlength != 0. In all such cases EPFL_ERROR should be set! */
            HCC_ASSERT(ep_info[ep].tlength==0);
            /* Otherwise send handshake. */
            ep_info[ep].state=EPST_STATUS_RX;
            /* Send a 0 byte long data0 packet as response.*/
            ready_ep_tx(ep);
            send_handshake(ep);
          }
        }
        break;
      case EPST_STATUS_RX:

        ep_info[ep].state=EPST_IDLE;
        /* In the OUT transfer status stage we sent a 0 byte long DATA 0 packet. */
        /* Tell application transfer ended. */
        if (ep_info[ep].data_func != (void *) 0)
        {
          /* We don't care about the return value at this point, since we already
             sent the status, and the transfer is already ended. */
          (void)(*ep_info[ep].data_func)();
          /* Disable callbacks. */
          ep_info[ep].data_func = (void *)0;
        }

        ready_ep_rx(ep);
        break;
      }
    }
    else if ((AT91C_UDP_CSR[ep] & AT91C_UDP_EPTYPE)
                ==AT91C_UDP_EPTYPE_BULK_OUT)
    {
      /* Handle reception. */
      _usb_receive(ep);
    }
    else if ((AT91C_UDP_CSR[ep] & AT91C_UDP_EPTYPE)
                   == AT91C_UDP_EPTYPE_BULK_IN)
    { /* This is an IN endpoint (TX) */
      _usb_send(ep);
    }

    else if ((AT91C_UDP_CSR[ep] & AT91C_UDP_EPTYPE)
                ==AT91C_UDP_EPTYPE_INT_OUT)
    {
      /* Handle reception. */
      _usb_receive(ep);
    }
    else if ((AT91C_UDP_CSR[ep] & AT91C_UDP_EPTYPE)
                   == AT91C_UDP_EPTYPE_INT_IN)
    { /* This is an IN endpoint (TX) */
      _usb_send(ep);
    }
    /* Clear interrupt request. */
    *AT91C_UDP_ICR = 1u<<ep;
  }
  isr_end:
    ;

	/* End the interrupt in the AIC. */
	AT91C_BASE_AIC->AIC_EOICR = 0;
}
/****************************** END OF FILE **********************************/

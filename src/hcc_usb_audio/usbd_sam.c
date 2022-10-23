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
* Váci út 110.
* Hungary
*
* Tel:  +36 (1) 450 1302
* Fax:  +36 (1) 450 1303
* http: www.hcc-embedded.com
* email: info@hcc-embedded.com
*
***************************************************************************/
#include <assert.h>
#include "src/usb-device/usb-drivers/common/usbdi.h"
#include "src/usb-device/usb-drivers/usbd_sam_config.h"
#include "src/lib/target/regs.h"
#include "src/lib/os/os.h"
#include "usbd_std_config.h"

#if USBD_SOFTMR_SUPPORT
#include "src/usb-device/usb-drivers/common/sof_timer.h"
#endif

#if USBD_STD_MAJOR != 6
#error "Invalid USBD_STD version."
#endif

#if USBD_MAJOR != 5
#error "Invalid USBD version."
#endif

#if USBD_HWAPI_MAJOR != 3
#error "Incorrect low-level driver version."
#endif

#define DEBUG_TRACE 0

#if DEBUG_TRACE==1
  #define MK_DBGTRACE(evt, inf) (dbg_htraces[trace_hndx].event=(evt), dbg_htraces[trace_hndx++].info=(inf))
#else
  #define MK_DBGTRACE(evt, inf)      ((void)0)
#endif

#define MIN(a, b)  ((a)<(b) ? (a) : (b))

#define EPFL_RX_BUF1 0x1
#define EPFL_NOT_STARTING 0x2

#define STP_REQU_TYPE()    (((hcc_u8*)(setup_data))[0])
#define STP_REQUEST()      (((hcc_u8*)(setup_data))[1])
#define STP_VALUE()        LE16(((hcc_u16*)(setup_data))[1])
#define STP_VALUE_LO()     (((hcc_u8*)(setup_data))[2])
#define STP_VALUE_HI()     (((hcc_u8*)(setup_data))[3])

#define STP_INDEX()        LE16(((hcc_u16*)(setup_data))[2])
#define STP_INDEX_LO()     (((hcc_u8*)(setup_data))[4])
#define STP_INDEX_HI()     (((hcc_u8*)(setup_data))[5])

#define STP_LENGTH()       LE16(((hcc_u16*)(setup_data))[3])
#define STP_LENGTH_LO()    (((hcc_u8*)(setup_data))[6])
#define STP_LENGTH_HI()    (((hcc_u8*)(setup_data))[7])

/* This shall return the first two character of the setup packet. */
#define STP_REQU16()       (((hcc_u16*)(setup_data))[0])

#define SET_CSR_BITS(ep, bits) \
  do {\
    int ctr=1000;\
    AT91C_UDP_CSR[(ep)] |= (bits);\
    while ((AT91C_UDP_CSR[(ep)] & (bits)) != (bits)\
            && ctr--)\
      ;\
  }while(0)


#define CLR_CSR_BITS(ep, bits) \
  do {\
    int ctr=1000;\
    AT91C_UDP_CSR[(ep)] &= ~(bits);\
    while ((AT91C_UDP_CSR[(ep)] & (bits)) != 0\
          && ctr--)\
      ;\
  }while(0)

typedef enum {
  evt_none=0,
  evt_get_stall,
  evt_clr_stall,
  evt_set_stall,
  evt_set_addr,
  evt_send_ok,
  evt_receive_ok,
  evt_abort,
  evt_add_ep,
  evt_remove_ep,
  evt_hw_init,
  evt_def_stat,
  evt_hw_start,
  evt_hw_stop,
  evt_hw_delete,
  evt_it,
  evt_event_set,
  evt_tx_packet,
  evt_rx_packet,
  evt_stp_host_abort,
  evt_stp_bad_length,
  evt_got_stp,
  evt_rx_host_abort,
  evt_rx_short_packet,
  evt_tx_short_packet,
  evt_dis_irq,
  evt_enb_tx_buf_2,
  evt_stall_it
} dbg_evt;

static hcc_u8 usb_state_bf_suspend;

#if DEBUG_TRACE==1
static struct {
  dbg_evt event;
  hcc_u16 info;
} dbg_htraces[0x100];

hcc_u8 trace_hndx=0;
#endif

static hcc_u32 setup_data[8/4];
os_it_info_t *udp_it;

static hcc_u8 pep2gndx[NO_OF_HW_EP];

int usbd_at_high_speed(void)
{
  return(0);
}

void usbd_get_setup_data(usbd_setup_data_t *stp)
{
  stp->bmRequestType=STP_REQU_TYPE();
  stp->bRequest=STP_REQUEST();
  stp->wValue=STP_VALUE();
  stp->wIndex=STP_INDEX();
  stp->wLength=STP_LENGTH();
}

void usbd_clr_stall(int ndx)
{
  int pep=usbd_ep_list[ndx].hw_info.pep;

  MK_DBGTRACE(evt_clr_stall, pep);

  *AT91C_UDP_RSTEP = (1<<pep);
  CLR_CSR_BITS(pep, (hcc_u32)AT91C_UDP_FORCESTALL);
  *AT91C_UDP_RSTEP = 0;
}

void usbd_set_stall(int ndx)
{
  int pep=usbd_ep_list[ndx].hw_info.pep;
  MK_DBGTRACE(evt_set_stall, pep);
  SET_CSR_BITS(pep, AT91C_UDP_FORCESTALL);

  /* Enable reception of next setup packet. */
  if (pep==0)
  {
    *AT91C_UDP_IER=BIT0;
  }
}

int usbd_get_stall(int ndx)
{
  int endx=usbd_ep_list[ndx].hw_info.pep;
  int r=AT91C_UDP_CSR[endx] & AT91C_UDP_FORCESTALL ? 1 : 0;
//  MK_DBGTRACE(evt_get_stall, endx|(r<<5));
  return(r);
}

void usbd_set_addr_pre(hcc_u8 daddr)
{
  (void)daddr;
  /* empty */
}

void usbd_set_addr_post(hcc_u8 daddr)
{
  MK_DBGTRACE(evt_set_addr, daddr);
  *AT91C_UDP_GLBSTATE  = AT91C_UDP_FADDEN;
  *AT91C_UDP_FADDR = (AT91C_UDP_FEN | daddr);
}

void usbd_set_cfg_pre(void)
{
  /* empty */
}

void usbd_set_cfg_post(void)
{
  *AT91C_UDP_GLBSTATE  = AT91C_UDP_FADDEN | AT91C_UDP_CONFG;
}

static void copy_to_ep(hcc_u8 ep, hcc_u8 *src, hcc_u16 length)
{
  hcc_u8 *end=src+length;

  /* Copy data into buffer(s). */
  while(src != end)
  {
    AT91C_UDP_FDR[ep]=*src++;
  }
}

int usbd_send(usbd_transfer_t *tr)
{
  int length;
  hcc_u8 *src_ptr;
  hcc_u32 left;

  usbd_ep_info_t *eph=usbd_ep_list+USBD_EPH_NDX(tr->eph);
  hcc_u8 pep=eph->hw_info.pep;

  MK_DBGTRACE(evt_send_ok, pep);

  /* Endpoint must have interrupts disabled here to avoid race problems. */
  assert((*AT91C_UDP_IMR & (1<<pep)) == 0);

  /* Set transfer state to busy. */
  tr->state=USBDTRST_BUSY;

  /* Number of bytes left from transfer. */
  left=tr->length-tr->csize;
  /* Length of data packet to be sent. */
  length=MIN(left, eph->psize);

  /* Pointer to packet data. */
  src_ptr=(hcc_u8*)(tr->buffer+tr->csize);

  /* If endpoint is double bufered. */
  if (pep != 0 && pep!=3)
  {
    /* If this is the start of the transfer. Prepare both buffers if there
       is enough data. */
    if (tr->csize==0)
    {
      /* Fill buffer 1. */
      copy_to_ep(pep, src_ptr, length);

      /* Ready just filled buffer. */
      SET_CSR_BITS(pep, AT91C_UDP_TXPKTRDY);

      left-=length;

      /* fill second buffer if there is enough data */
      if (left)
      {
        int l=MIN(left, eph->psize);
        src_ptr+=length;
        copy_to_ep(pep, src_ptr, l);
        eph->hw_info.db=1;
      }
    }
    /* Transfer is already ongoing. Just fill current buffer. Interrupt will
       enable it later. */
    else
    {
      /* There is a packet pending in the other buffer. Since the tx it disables
         additional interrupts to this endpoint, transfer status values are
         behind the real ones. To avoid sending the same data twice we must do
         corrections. */
      /* Is there something to be sent? */
      if( left > eph->psize )
      {
        /* correct number of bytes to handle */
        left-=eph->psize;
        /* correct source pointer */
        src_ptr+=eph->psize;
        /* correct packet length */
        length=MIN(left, eph->psize);
        /* fill buffer */
        copy_to_ep(pep, src_ptr, length);
      }
      else
      {
        eph->hw_info.db=0;
      }
    }
  }
  else
  {
    /* Fill buffer */
    copy_to_ep(pep, src_ptr, length);
    /* Ready just filled buffer. */
    SET_CSR_BITS(pep, AT91C_UDP_TXPKTRDY);
  }
  /* Enable interrupt generation of endpoint. */
  *AT91C_UDP_IER=1<<pep;
  return(0);
}

static void usb_copy_from_ep(hcc_u8 ep, hcc_u8* dst, int length)
{
  if (length==0)
  {
    volatile hcc_u8 x=AT91C_UDP_FDR[ep];
    (void)x;
  }
  else
  {
    hcc_u8 *end=dst+length;
    while(dst != end)
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
    usbd_ep_list[ep].hw_info.flags &= ~EPFL_RX_BUF1;
    CLR_CSR_BITS(ep, (hcc_u32)AT91C_UDP_RX_DATA_BK0);
    break;
  case AT91C_UDP_RX_DATA_BK1:
  usebuf1:
    usbd_ep_list[ep].hw_info.flags |= EPFL_RX_BUF1;
    CLR_CSR_BITS(ep, (hcc_u32)AT91C_UDP_RX_DATA_BK1);
    break;
  case (AT91C_UDP_RX_DATA_BK1 | AT91C_UDP_RX_DATA_BK0):
    if (usbd_ep_list[ep].hw_info.flags & EPFL_RX_BUF1)
    {
      goto usebuf0;
    }
    else
    {
      goto usebuf1;
    }
  case AT91C_UDP_RXSETUP:
    /* For control endpoints we may have to clear the setup packed received flag. */
    CLR_CSR_BITS(ep, (hcc_u32)AT91C_UDP_RXSETUP);
    break;
  default:
    assert(0);
  }
}

int usbd_receive(usbd_transfer_t *tr)
{
  hcc_u8 pep=usbd_ep_list[USBD_EPH_NDX(tr->eph)].hw_info.pep;

  MK_DBGTRACE(evt_receive_ok, pep);

  /* Set transfer state to busy. */
  tr->state=USBDTRST_BUSY;

  *AT91C_UDP_IER=(1<<pep);
  return(0);
}

int usbd_abort(usbd_ep_handle_t ep)
{
  hcc_u8 pep=usbd_ep_list[USBD_EPH_NDX(ep)].hw_info.pep;

  MK_DBGTRACE(evt_abort, pep);
  *AT91C_UDP_RSTEP |= 1<<pep;
  return(OS_SUCCESS);
}

int usbd_add_ep(int ndx)
{
  usbd_ep_info_t *eph=usbd_ep_list+ndx;
  hcc_u8 pep=eph->addr & 0x7f;
  hcc_u32 csrval;

  /* Check if endpoint is already configured by
    the STD module. */
  assert(eph->psize);

  /* If the endpoint address is invalid. */
  if (pep >= NO_OF_HW_EP)
  {
    return(OS_ERR);
  }

  MK_DBGTRACE(evt_add_ep, 0);
  /* Reset endpoint. */
  *AT91C_UDP_RSTEP = (1<<pep);

  pep2gndx[pep]=ndx;
  eph->hw_info.pep=pep;
  eph->hw_info.db=0;
  eph->hw_info.flags=EPFL_RX_BUF1;

  *AT91C_UDP_RSTEP = 0;

  if ((eph->ep_type == EPD_ATTR_CTRL) || !(eph->addr & EPD_DIR_TX))
  {
    csrval=(eph->ep_type<<8) | AT91C_UDP_EPEDS;
    AT91C_UDP_CSR[pep] = csrval;
    while((AT91C_UDP_CSR[pep] & AT91C_UDP_EPEDS)==0)
      ;
  }
  else
  {
   csrval=((eph->ep_type|4)<<8) | AT91C_UDP_EPEDS;
    AT91C_UDP_CSR[pep] = csrval;
    while((AT91C_UDP_CSR[pep] & AT91C_UDP_EPEDS)==0)
      ;
  }

  /* Enable control ep interrupt */
  if (eph->ep_type == AT91C_UDP_EPTYPE_CTRL)
  {
    *AT91C_UDP_IER=(1<<pep);
  }
  return(OS_SUCCESS);
}

void usbd_remove_ep(int ndx)
{
  hcc_u8 pep=usbd_ep_list[ndx].hw_info.pep;

  MK_DBGTRACE(evt_remove_ep, pep);

  pep2gndx[pep]=0xff;

  *AT91C_UDP_IDR=1<<pep;
  CLR_CSR_BITS(pep, (hcc_u32)AT91C_UDP_EPEDS);
}

int usbd_hw_init(usbd_hw_init_info_t *hi)
{
  MK_DBGTRACE(evt_hw_init, 0);

  udp_it=&hi->it_info;
  os_isr_init(udp_it);

  /* Enable USB clock. Note: the code assumes UDPCK has the correct 48MHz value when
     this function is called (see USBDIV in CKGR_PLLR register). */
    /* Enable USB clock in system clock enable register. */
  *AT91C_PMC_SCER=AT91C_PMC_UDP;
    /* and in peripheral clock enable register. */
  *AT91C_PMC_PCER = (1u<<AT91C_ID_UDP);

  /* Disable pull-up (disconnect from USB), and enable data pins. */
  *AT91C_UDP_TXVC = 0;
#if !ON_CHIP_PULL_UP
  usbd_pup_on_off(0);
#endif

  /* Disable all USB interrupts. */
  *AT91C_UDP_IDR = -1u;
  /* Clear all pending USB interrupts. */
  *AT91C_UDP_ICR = -1u;

#if USBD_SOFTMR_SUPPORT
  return softmr_init();
#else
  return OS_SUCCESS;
#endif
}

static void enter_default_state(void)
{
  MK_DBGTRACE(evt_def_stat, 0);
  /* Clear all pending USB interrupts. */
  *AT91C_UDP_ICR = -1u;
  *AT91C_UDP_IER = AT91C_UDP_ENDBUSRES | AT91C_UDP_WAKEUP
#if USBD_SOFTMR_SUPPORT
     | AT91C_UDP_SOFINT
#endif

     | AT91C_UDP_RXSUSP;
  /* Disable function endpoints, exit adressed and configured state. */
  *AT91C_UDP_GLBSTATE  = 0;
  /* Set address to 0. */
  *AT91C_UDP_FADDR = (AT91C_UDP_FEN | 0);
  /* This call assumes usbd_std_start() already configured EP0 in
    the global endpoint list. */
  usbd_add_ep(0);

  usbd_state=USBDST_DEFAULT;
}

int usbd_hw_start(void)
{
  MK_DBGTRACE(evt_hw_start, 0);

  os_int_disable();

#if ON_CHIP_PULL_UP
  *AT91C_UDP_TXVC = AT91C_UDP_PUON;
#else
  usbd_pup_on_off(1);
#endif
  /* Enable pull-up and USB data pins. */
  enter_default_state();

  os_isr_enable(udp_it);
#if USBD_SOFTMR_SUPPORT
  softmr_start();
#endif
  os_int_restore();

  return(OS_SUCCESS);
}

int usbd_hw_stop(void)
{
  MK_DBGTRACE(evt_hw_stop, 0);

  (void)os_isr_disable(udp_it);
  /* Disable all USB interrupts. */
  *AT91C_UDP_IDR = -1u;
  /* Clear all pending USB interrupts. */
  *AT91C_UDP_ICR = -1u;
  /* Disable pull-up. */
#if ON_CHIP_PULL_UP
  *AT91C_UDP_TXVC &= ~(hcc_u32)AT91C_UDP_PUON;
#else
  usbd_pup_on_off(0);
#endif
#if USBD_SOFTMR_SUPPORT
  return softmr_stop();
#else
  return OS_SUCCESS;
#endif
}

int usbd_hw_delete(void)
{
  MK_DBGTRACE(evt_hw_delete, 0);

  /* Disable pull-up (disconnect from USB), and turn off analog parts. */
  *AT91C_UDP_TXVC = AT91C_UDP_TXVDIS;

  /* Disable module clock. */
  *AT91C_PMC_PCDR = (1u<<AT91C_ID_UDP);
  *AT91C_PMC_SCDR=AT91C_PMC_UDP;

  return(OS_SUCCESS);
}

static void handle_rx_ev(usbd_ep_info_t *eph)
{
  hcc_u8 pep=eph->hw_info.pep;
  usbd_transfer_t *tr=eph->tr;

  if (AT91C_UDP_CSR[pep] & AT91C_UDP_RXSETUP)
  {
    int length;
    assert(pep==0);

    if (tr)
    {
      /* If host aborts ongoing transfer by sending next setup packet. */
      MK_DBGTRACE(evt_stp_host_abort, 0);
      tr->state=USBDTRST_COMM;
      if (tr->event)
      {
        os_event_set(*tr->event);
      }
    }
    length=(AT91C_UDP_CSR[0] & AT91C_UDP_RXBYTECNT)>>16;
    assert(length == 8);
    if (length != 8)
    {
      MK_DBGTRACE(evt_stp_bad_length, 0);
      usb_copy_from_ep(0, 0, 0);
      usbd_set_stall(0);
    }
    else
    {
      MK_DBGTRACE(evt_got_stp, 0);
      usb_copy_from_ep(0, (hcc_u8*)setup_data, 8);
      if (STP_REQU_TYPE() & USBRQT_DIR_IN)
      {
        SET_CSR_BITS(0, AT91C_UDP_DIR);
      }
      else
      {
        CLR_CSR_BITS(0, (hcc_u32)AT91C_UDP_DIR);
      }

      os_event_set_int(usbd_stprx_event);
      *AT91C_UDP_IDR=1<<0;
    }
    return;
  }

  if (AT91C_UDP_CSR[pep] & (AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1))
  {
    int length;

    if (NULL==tr)
    {
      assert(EPD_ATTR_ISO == eph->ep_type);
      if (eph->fifo)
      {
        static void fifo_rx_ready_cb(hcc_u32 param);
        /* Disable endpoint interrupt. rx_ready_cb() will re-enable it if the FIFO
           has free space available. */
        *AT91C_UDP_IDR=1<<pep;
        fifo_rx_ready_cb(eph-usbd_ep_list);
      }
      return;
    }
    else
    {
      if (pep == 0 && tr->dir == USBDTRDIR_IN)
      {
        MK_DBGTRACE(evt_rx_host_abort, 0);
        length=(AT91C_UDP_CSR[0] & AT91C_UDP_RXBYTECNT)>>16;
        if (length != 0)
        {
          usbd_set_stall(0);
        }
        /* Do this to clear the interrupt. */
        usb_copy_from_ep(0, 0, 0);
        tr->state=USBDTRST_COMM;
      }
      else
      {
        length=(AT91C_UDP_CSR[pep] & AT91C_UDP_RXBYTECNT)>>16;
        usb_copy_from_ep(pep, ((hcc_u8*)tr->buffer)+tr->csize
                         , length);
        tr->csize+=length;

        if (length != eph->psize)
        {
          MK_DBGTRACE(evt_rx_short_packet, pep);
          tr->state=USBDTRST_SHORTPK;
        }
        else
        {
          tr->state=USBDTRST_CHK;
        }

        if (tr->dir!=USBDTRDIR_HS_IN)
        {
           MK_DBGTRACE(evt_dis_irq, pep);
          *AT91C_UDP_IDR=1<<pep;
        }
      }

      if (tr->event)
      {
        os_event_set_int(*tr->event);
      }
      return;
    }
  }
  /* Execution shall never get to this line. */
  assert(0);
}

static void handle_tx_ev(usbd_ep_info_t *eph)
{
  hcc_u8 pep=eph->hw_info.pep;
  usbd_transfer_t *tr=eph->tr;
  hcc_u16 psize;

  MK_DBGTRACE(evt_tx_packet, pep);

  if (eph->hw_info.db)
  {
    /* Ready second buffer. */
    SET_CSR_BITS(pep, AT91C_UDP_TXPKTRDY);
    MK_DBGTRACE(evt_enb_tx_buf_2, pep);
  }

  if (NULL==tr)
  {
    if (NULL!=eph->fifo)
    {
      static void fifo_tx_ready_cb(hcc_u32 param);
      /* Disable endpoint interrupt. rx_ready_cb() will re-enable it if the FIFO
         has free space available. */
      *AT91C_UDP_IDR=1<<pep;
      fifo_tx_ready_cb(eph-usbd_ep_list);
    }
  }
  else
  {
    /* Advance transfer status. */
    psize=MIN(tr->length-tr->csize, eph->psize);
    tr->csize+=psize;
    if (psize != eph->psize)
    {
      MK_DBGTRACE(evt_tx_short_packet, pep);
      tr->state=USBDTRST_SHORTPK;
    }
    else
    {
      tr->state=USBDTRST_CHK;
    }

    if (tr->dir != USBDTRDIR_HS_OUT)
    {
      MK_DBGTRACE(evt_dis_irq, pep);
      *AT91C_UDP_IDR=1<<pep;
    }

    CLR_CSR_BITS(pep, (hcc_u32)AT91C_UDP_TXCOMP);
    if (tr->event)
    {
      os_event_set_int(*tr->event);
    }
  }
}

OS_ISR_FN(usbd_it_handler)
{
  hcc_u16 istr;
  /* Save irq USB status. Note: ISR bits are
     not directly routed to the AIC. Thus
     ISR must be masked with IMR to avoid
     process masked interrupts if another
     interrupt source strikes. */
  istr = *AT91C_UDP_ISR;
  istr &= *AT91C_UDP_IMR;
  MK_DBGTRACE(evt_it, istr);


#if USBD_SOFTMR_SUPPORT
  if (istr & AT91C_UDP_SOFINT)
  {
    *AT91C_UDP_ICR = AT91C_UDP_SOFINT;
    istr &= ~AT91C_UDP_SOFINT;
    softmr_tick(*AT91C_UDP_NUM & AT91C_UDP_FRM_NUM);
  }
#endif

  if (istr & (AT91C_UDP_ENDBUSRES | AT91C_UDP_WAKEUP
              | AT91C_UDP_RXSUSP))
  {
    if (istr & AT91C_UDP_ENDBUSRES)
    {
      /* Clear IT flag. */
      *AT91C_UDP_ICR = AT91C_UDP_ENDBUSRES;
      istr &= ~AT91C_UDP_ENDBUSRES;

      /* Enter default state. */
      enter_default_state();
      os_event_set_int(usbd_bus_event);
      goto isr_end;
    }

    if (istr & AT91C_UDP_WAKEUP)
    {
      *AT91C_UDP_ICR = AT91C_UDP_WAKEUP;
      istr &= ~AT91C_UDP_WAKEUP;

      usbd_state=usb_state_bf_suspend;
    }

    if (istr & AT91C_UDP_RXSUSP)
    {
      *AT91C_UDP_ICR = AT91C_UDP_RXSUSP;
      istr &= ~AT91C_UDP_RXSUSP;

      usb_state_bf_suspend=usbd_state;
      usbd_state=USBDST_SUSPENDED;
    }
    os_event_set_int(usbd_bus_event);
  }

  while (istr & ((1<<NO_OF_HW_EP)-1))
  {
    hcc_u8 ep;
    int mask=1;

    for(ep=0; ep<NO_OF_HW_EP;ep++, mask<<=1)
    {
      if (istr & mask)
      {
        break;
      }
    }
    assert(ep<NO_OF_HW_EP);
    istr &= ~mask;

    if (AT91C_UDP_CSR[ep] & AT91C_UDP_ISOERROR)
    {
      MK_DBGTRACE(evt_stall_it, ep);
      CLR_CSR_BITS(ep, (hcc_u32)AT91C_UDP_ISOERROR);

/*      if (! (AT91C_UDP_CSR[ep] & (AT91C_UDP_RX_DATA_BK0
           | AT91C_UDP_RX_DATA_BK1 | AT91C_UDP_RXSETUP
           | AT91C_UDP_TXCOMP)))
      {
        MK_DBGTRACE(evt_stall_it_only, ep);
        goto isr_end;
      }
*/
    }

    assert(pep2gndx[ep] < NO_OF_HW_EP);

    if (AT91C_UDP_CSR[ep] & (AT91C_UDP_RX_DATA_BK0
                   | AT91C_UDP_RX_DATA_BK1 | AT91C_UDP_RXSETUP))
    {/* This is an RX packet.*/
      handle_rx_ev(usbd_ep_list+pep2gndx[ep]);
    }
    else if (AT91C_UDP_CSR[ep] & AT91C_UDP_TXCOMP)
    {
      handle_tx_ev(usbd_ep_list+pep2gndx[ep]);
    }
  }
  isr_end:
    ;
  /* It seems this interrupt can not be disabled. We don't use it
     but it needs to be cleared or USB interrupts will be fired
     endless. */
  *AT91C_UDP_ICR=AT91C_UDP_RXRSM;
  istr &= ~AT91C_UDP_RXRSM;
  assert(istr==0);
}

static void fifo_tx_ready_cb(hcc_u32 param)
{
  rngbuf_item_t *p;
  int r=0;
  int pep=usbd_ep_list[param].hw_info.pep;

  /* Firast check if endpoint is busy or not. */
  if (!(usbd_ep_list[param].hw_info.flags & EPFL_NOT_STARTING)
      || AT91C_UDP_CSR[param] & AT91C_UDP_TXCOMP)
  {
    /* get next chunk */
    r=rngbuf_next_filled(usbd_ep_list[param].fifo, &p);

    /* if we have data to be sent */
    if (NULL != p)
    {
      usbd_ep_list[param].hw_info.flags = EPFL_NOT_STARTING;
      copy_to_ep(pep, p->data.data, p->data.nbytes);
      /* Ready just filled buffer. */
      SET_CSR_BITS(pep, AT91C_UDP_TXPKTRDY);
      CLR_CSR_BITS(pep, (hcc_u32)AT91C_UDP_TXCOMP);
      rngbuf_step(usbd_ep_list[param].fifo, p);
    }
  }

  /* If more data needs to be sent, re enable interrupt. Interrupt will
     trigger the next send. */
  if (0==r)
  {
    /* Enable interrupt generation of endpoint. */
    *AT91C_UDP_IER=1<<pep;
  }
}

static void fifo_rx_ready_cb(hcc_u32 param)
{
  hcc_u16 length;
  rngbuf_item_t *p;
  int r=0;
  int pep=usbd_ep_list[param].hw_info.pep;
  /* If endpoint has data pending, push it to FIFO. */
  while (AT91C_UDP_CSR[pep] & (AT91C_UDP_RX_DATA_BK1 | AT91C_UDP_RX_DATA_BK0))
  {
    length=(AT91C_UDP_CSR[pep] & AT91C_UDP_RXBYTECNT)>>16;
    r=rngbuf_next_free(usbd_ep_list[param].fifo, &p);
    if (NULL != p)
    {
      usb_copy_from_ep(pep, p->data.data, length);
      p->data.nbytes=length;
      rngbuf_step(usbd_ep_list[param].fifo, p);
    }
    else
    {
      break;
    }
  }

  /* 1: Function called by FIFO:
        this case if the while loop was entered above, then r tells us
          if there is space in the fifo or not.
        if the while loop was not entered, then we have space in the FIFO since
        the call-back is not called otherwise.
     2: Function called by it handler:
        the while loop must be entered this case, since we have received data.
        Thus r has valid information about the FIFO full state.
     3: Function called from usbd_add_fifo
        We are just starting up, thus the FIFO must have free space. The same
        applies as for case 1. */
  if (0==r)
  {/* Enable interrupt generation of endpoint. */
    *AT91C_UDP_IER=1<<pep;
  }
}

int usbd_add_fifo(int index)
{
  if (usbd_ep_list[index].addr & 0x80)
  {
    rngbuf_set_outcb(usbd_ep_list[index].fifo, fifo_tx_ready_cb, index);
    fifo_tx_ready_cb(index);
  }
  else
  {
    rngbuf_set_incb(usbd_ep_list[index].fifo, fifo_rx_ready_cb, index);
    fifo_rx_ready_cb(index);
    *AT91C_UDP_IER=1<<usbd_ep_list[index].hw_info.pep;
  }
  return(OS_SUCCESS);
}

int usbd_drop_fifo(int index)
{
  *AT91C_UDP_IDR = 1 << usbd_ep_list[index].hw_info.pep;
  if (usbd_ep_list[index].addr & 0x80)
  {
    rngbuf_set_outcb(usbd_ep_list[index].fifo, NULL, 0);
  }
  else
  {
    rngbuf_set_incb(usbd_ep_list[index].fifo, NULL, 0);
  }
  return(OS_SUCCESS);
}

hcc_u16 usbd_get_frame_ctr(void)
{
  return(AT91C_UDP_FRM_NUM & AT91C_UDP_FRM_NUM);
}
/****************************** END OF FILE **********************************/

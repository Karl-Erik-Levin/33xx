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
#include <assert.h>
#include <string.h>

#include "sof_timer.h"
#include "src/lib/os/os.h"
#include "src/lib/compiler/compiler.h"
#include "src/usb-device/usb-drivers/common/usbd.h"

#define SOFTMR_MAX_EVENTS  5
#define FRAME_CTR_MASK  ((1u<<11)-1)

/* Timer event descriptor */
struct
{
  hcc_u32 due_time;
  hcc_u32 period;
  softmr_event_func_t cb;
  int cb_param;
  int used;
} event_list[SOFTMR_MAX_EVENTS];

hcc_u32 ctr;
hcc_u16 last_fctr_val;

int softmr_init(void)
{
  ctr=0;
  memset(event_list, 0, sizeof(event_list));
  return OS_SUCCESS;
}

int softmr_start(void)
{
  ctr=0;
  last_fctr_val=usbd_get_frame_ctr() & FRAME_CTR_MASK;
  return OS_SUCCESS;
}

int softmr_stop(void)
{
  return OS_SUCCESS;
}

int softmr_add_event(softmr_event_func_t cb_fn, int cb_param, hcc_u32 period)
{
  int ndx;

  for(ndx=0; ndx<SOFTMR_MAX_EVENTS; ndx++)
  {
    if (0==event_list[ndx].used)
    {
      event_list[ndx].period=period;
      event_list[ndx].due_time=ctr+period;
      event_list[ndx].cb=cb_fn;
      event_list[ndx].cb_param=cb_param;
      event_list[ndx].used=1;
      break;
    }
  }

  if (ndx<SOFTMR_MAX_EVENTS)
  {
    return ndx;
  }

  return SOFTMR_INVALID_EVENT;
}

int softmr_del_event(softmr_event_handle_t evh)
{
  if (SOFTMR_INVALID_EVENT == evh)
  {
    return OS_ERR;
  }
  event_list[evh].used=0;
  return OS_SUCCESS;
}

int softmr_reset_event(softmr_event_handle_t evh)
{
  if (SOFTMR_INVALID_EVENT == evh)
  {
    return OS_ERR;
  }
  event_list[evh].due_time=ctr+event_list[evh].period;
  return OS_SUCCESS;
}

void softmr_tick(hcc_u16 sof_ctr)
{
  int ndx;

  /* Increment mS counter. */
  ctr+=FRAME_CTR_MASK & (sof_ctr-last_fctr_val);
  last_fctr_val=sof_ctr;

  /* Check for due events. */
  for(ndx=0; ndx<SOFTMR_MAX_EVENTS; ndx++)
  {
     if (event_list[ndx].used
         && event_list[ndx].due_time < ctr)
     {
       event_list[ndx].due_time+=event_list[ndx].period;
       (*event_list[ndx].cb)(event_list[ndx].cb_param);
     }
  }
}

/****************************** END OF FILE **********************************/

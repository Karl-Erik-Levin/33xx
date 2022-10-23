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
#ifndef _SOF_TIMER_H_
#define _SOF_TIMER_H_

#include "src/lib/compiler/compiler.h"
#include "src/lib/os/os.h"

#ifdef __cplusplus
#extern "C" {
#endif

#define SOFTMR_INVALID_EVENT -1

typedef int softmr_event_handle_t;
typedef void (*softmr_event_func_t)(int param);

int softmr_init(void);
int softmr_start(void);
softmr_event_handle_t softmr_add_event(softmr_event_func_t cb_fn, int param, hcc_u32 period);
int softmr_reset_event(softmr_event_handle_t evh);
int softmr_del_event(softmr_event_handle_t evh);
int softmr_stop(void);

void softmr_tick(hcc_u16 sof_ctr);

#ifdef __cplusplus
}
#endif

#endif
/****************************** END OF FILE **********************************/

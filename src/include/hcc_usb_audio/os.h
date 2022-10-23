/****************************************************************************
 *
 *            Copyright (c) 2007-2009 by HCC Embedded
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
#ifndef _OS_H_
#define _OS_H_

#include "src/lib/compiler/compiler.h"
#include "src/lib/target/regs.h"
#include "application/os_cfg.h"
#include "platform/FreeRTOS/FreeRTOS.h"
#include "platform/FreeRTOS/semphr.h"
#include "platform/FreeRTOS/task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OS_ISR_DEF(name)          extern void name(void)
#define OS_ISR_FN(name)           void name(void)
#define OS_TASK_DEF(name)         portTASK_FUNCTION_PROTO(name, p)
#define OS_TASK_FN(name,param)    portTASK_FUNCTION(name, param)

#define OS_MUTEX_TYPE             xSemaphoreHandle

#define OS_EVENT_BIT_TYPE         xSemaphoreHandle
#define OS_EVENT_TYPE             xSemaphoreHandle

typedef struct {
   hcc_u8 periph_id;
   hcc_u8 prio;
   void (*handler)(void);
} os_it_info_t;

#ifdef __cplusplus
}
#endif

#include "src/lib/os/os_common.h"

#if OS_API_MAJOR != 2
#error "Incorrect OS API version."
#endif

#endif
/****************************** END OF FILE **********************************/

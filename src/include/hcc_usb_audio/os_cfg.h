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
#ifndef _PORT_OS_H
#define _PORT_OS_H

#ifdef __cplusplus
extern "C" {
#endif


/* Interrupt handlers run polled if this is 0. */
#define OS_INTERRUPT_ENABLE		1
/* Set to 1 if tasks run in cooperative mode. */
#define OS_TASK_POLL_MODE		  0
/* Max number of mutexes available. */
#define OS_MUTEX_COUNT			3
/* Max number of events available. */
#define OS_EVENT_BIT_COUNT		16

#ifdef __cplusplus
}
#endif


#endif
/****************************** END OF FILE **********************************/

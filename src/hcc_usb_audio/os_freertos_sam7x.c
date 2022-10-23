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
#include <assert.h>
#include "os.h"
#include "src/lib/target/target.h"
#include "src/lib/compiler/compiler.h"

void os_int_enable(void)
{
  taskENABLE_INTERRUPTS();
}


void os_int_disable(void)
{
  taskDISABLE_INTERRUPTS();
}

void os_int_restore(void)
{
#if OS_INTERRUPT_ENABLE
  taskENABLE_INTERRUPTS();
#endif
}

#if OS_INTERRUPT_ENABLE
#define os_idle()
#else
#endif

int os_isr_init (os_it_info_t *it)
{
  if (it->periph_id==AT91C_ID_UDP)
  {
    extern void usbDrvISREntry(void);
    AT91C_AIC_SVR[it->periph_id]=(hcc_u32)usbDrvISREntry;
    AT91C_AIC_SMR[it->periph_id]=it->prio;
  }
  return OS_SUCCESS;
}

int os_isr_enable (os_it_info_t *it)
{
  *AT91C_AIC_IECR=(1<<it->periph_id);
  return OS_SUCCESS;
}

int os_isr_disable (os_it_info_t *it)
{
  *AT91C_AIC_IDCR=(1<<it->periph_id);
  return OS_SUCCESS;
}

#if OS_MUTEX_COUNT
int os_mutex_create(OS_MUTEX_TYPE **pmutex)
{
  *pmutex = xSemaphoreCreateMutex();
  if (NULL==(OS_MUTEX_TYPE)*pmutex)
  {
    return OS_ERR;
  }
  return OS_SUCCESS;
}

int os_mutex_delete(OS_MUTEX_TYPE *pmutex)
{
  (void)pmutex;
  /* There is no way in FreeRTOS to destroy a mutex. */
  return OS_SUCCESS;
}

/* Mutex get always succeeds. */
int os_mutex_get(OS_MUTEX_TYPE *pmutex)
{
  if (pdTRUE==xSemaphoreTake(pmutex, portMAX_DELAY))
  {
    return OS_SUCCESS;
  }
  return OS_ERR;
}

int os_mutex_put(OS_MUTEX_TYPE *pmutex)
{
  (void)xSemaphoreGive(pmutex);
  return OS_SUCCESS;
}
#endif

#if OS_EVENT_BIT_COUNT
int os_event_create(OS_EVENT_BIT_TYPE *event_bit)
{
  vSemaphoreCreateBinary(*event_bit);
  if (NULL==*event_bit)
  {
    return(OS_ERR);
  }
  xSemaphoreTake(*event_bit, portMAX_DELAY);
  return(OS_SUCCESS);
}

int os_event_delete(OS_EVENT_BIT_TYPE event_bit)
{
  (void)event_bit;
  /* No way to destroy a semaphore in FreeRTOS. */
  return(OS_SUCCESS);
}

int os_event_get(OS_EVENT_BIT_TYPE event_bit)
{
  if (pdTRUE==xSemaphoreTake(event_bit, portMAX_DELAY))
  {
    return(OS_SUCCESS);
  }
  return(OS_ERR);
}

int os_event_set(OS_EVENT_BIT_TYPE event_bit)
{
  (void)xSemaphoreGive(event_bit);
  return OS_SUCCESS;
}

int os_event_set_int(OS_EVENT_BIT_TYPE event_bit)
{
  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

  (void)xSemaphoreGiveFromISR(event_bit, &xHigherPriorityTaskWoken);
  if (pdFALSE != xHigherPriorityTaskWoken)
  {
    portYIELD_FROM_ISR();
  }

  return OS_SUCCESS;
}

#endif

void AT91F_Spurious_handler(void)
{
  static hcc_u32 nspurint=0;
  nspurint++;
}

__arm __fiq void AT91F_Default_IRQ_handler(void)
{
  for(;;)
    ;
}

__irq __arm void AT91F_Default_FIQ_handler(void)
{
  for(;;)
    ;
}

int os_init (void)
{
  return OS_SUCCESS;
}

int os_start (void)
{
  return OS_SUCCESS;
}

__irq __arm void Undefined_Handler(void)
{
  while(1)
    ;
}

__irq __arm void Prefetch_Handler(void)
{
  while(1)
    ;
}

__irq __arm void Abort_Handler(void)
{
  int a=1;
  while(a)
    ;
}

/****************************** END OF FILE **********************************/

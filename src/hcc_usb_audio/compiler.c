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


#include "compiler.h"

int _disable_interrupt(void)
{
  int r=__get_interrupt_state();
  __disable_interrupt();
  return(r);
}
void _restore_interrupt(int im)
{
  __set_interrupt_state(im);
}

/*
** Empty low-level function for IAR DLib. This makes it possible to use both
** hardware endpoints for our needs. (C-Spy I/O does not need a break point.)
**
*/
size_t __write(int handle,
               const unsigned char *buf,
               size_t bufSize)
{
  (void)handle;
  (void)buf;
  (void)bufSize;
  return(bufSize);
}

/*
** Empty low-level function for IAR DLib. This makes it possible to use both
** hardware endpoints for our needs. (C-Spy I/O does not need a break point.)
**
*/
size_t __read(int handle,
              unsigned char *buf,
              size_t bufSize)
{
  (void)handle;
  (void)buf;
  (void)bufSize;
  return(0);
}
/****************************** END OF FILE **********************************/

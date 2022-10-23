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
#include "board.h"
#include "src/lib/target/target.h"

int board_init(void)
{
  /* empty */
  return(0);
}

int board_start(void)
{
  /* empty */
  return(0);
}

int board_stop(void)
{
  /* empty */
  return(0);
}

int board_delete(void)
{
  /* Switch back GPIO lines controlling leds to inputs.*/
  return(0);
}

void pup_on_off(int on)
{
  (void)on;
}

void led_on(int no)
{
  (void)no;
}

void led_off(int no)
{
  (void)no;
}

void led_toggle(int no)
{
  (void)no;
}

int read_switch(int no)
{
  (void)no;
  return 0;
}

/****************************** END OF FILE **********************************/

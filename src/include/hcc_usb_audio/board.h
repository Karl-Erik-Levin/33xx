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


#ifndef _BOARD_H_
#define _BOARD_H_

#ifdef _cplusplus
extern "C" {
#endif

#define NO_OF_LED 0
#define NO_OF_SW  0

#define EXT_CLOCK_FREQ 18423000ul

/* led, sw handling, EMI inicialization, external clock definitions */
extern int board_init(void);
extern int board_start(void);
extern int board_stop(void);
extern int board_delete(void);

extern void pup_on_off(int on);
extern void led_on(int no);
extern void led_off(int no);
extern void led_toggle(int no);

extern int read_switch(int no);

#ifdef _cplusplus
}
#endif

#endif

/****************************** END OF FILE **********************************/

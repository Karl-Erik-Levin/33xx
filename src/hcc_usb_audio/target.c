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


#include "src/lib/board/board.h"
#include "src/lib/os/os.h"
#include "src/lib/target/target.h"

OS_ISR_FN(dummy_irq)
{
  for(;;)
    ;
}
/*****************************************************************************
 * Name:
 *    init_aic
 * In:
 *    N/A
  * Out:
 *    N/A
 *
 * Description:
 *    Configure Advanced Interrupt Controller module.
 *
 * Assumptions:
 *
 *****************************************************************************/
static void aic_init(void)
{
#ifndef NDEBUG
  int x;
#endif

  /* enable AIC clock, */
  *AT91C_PMC_PCER= (1u<<AT91C_ID_FIQ) | (1u<<AT91C_ID_IRQ0) | (1u<<AT91C_ID_IRQ1);

  /* Configure the Advanced Interrupt Controller. */
    /* Disable all interrupts */
  *AT91C_AIC_IDCR = -1u;
    /* Clear all penging edge sensitive interrupts. */
  *AT91C_AIC_ICCR = -1u;

  /* Set FIQ vector address. */
  AT91C_AIC_SVR[AT91C_ID_FIQ] =(hcc_u32) dummy_irq;

  /* Set spurious vector. */
  *AT91C_AIC_SPU = (hcc_u32) dummy_irq;

#ifndef NDEBUG
  /* Put AIC to protected mode. Can be remover if not debugging. */
  *AT91C_AIC_DCR=AT91C_AIC_DCR_PROT;

  /* Clear AIC state machine. If you stop debugging, or restart your application
     while an interrupt routine is executed, then the internal stack of the AIC
     may get corrupted. This loop will fix it. */
  for(x=0; x < 32; x++)
  {
    *AT91C_AIC_EOICR=0;
  }
#endif
}

/*****************************************************************************
 * Name:
 *    init_clocks
 * In:
 *    N/A
  * Out:
 *    N/A
 *
 * Description:
 *    Configure on-chip clocks.
 *
 * Assumptions:
 *
 *****************************************************************************/
static void clock_init(void)
{
  /* Set flash waint states to 2. */
  *AT91C_MC_FMR = ((AT91C_MC_FMCN)&(50 <<16)) | AT91C_MC_FWS_1FWS;

  /* initialize MCU clock. */
  /* Start main oscillator. (maximum startup time) */
  *AT91C_CKGR_MOR =((0xff << 8) | AT91C_CKGR_MOSCEN);
    /* Wait till main OSC is stable. */
  while((*AT91C_PMC_SR & AT91C_PMC_MOSCS) == 0)
    ;
  /* Switch to external clock source. */
  *AT91C_PMC_MCKR =  AT91C_PMC_CSS_MAIN_CLK;
  while(!(*AT91C_PMC_SR & AT91C_PMC_MCKRDY))
    ;

  /* Note: to use USB 48, 96 or 192 MHz pll clock needs to be set. */

#if (EXT_CLOCK_FREQ == 18423000ul) && (MCK == 48000000ul) && (PLLCK == 96000000ul)
  *AT91C_PMC_PLLR = AT91C_CKGR_USBDIV_1 /*USB clock = PLL freq /2 */
                    | ((73-1)<<16)          /* mul = 73 */
                    | AT91C_CKGR_OUT_0  /* 80 MHZ < pll freq < 160MHz */
                    | (0x3f<<8)         /* startup time = maximum */
                    | 14;                /*div = 14*/

#elif (EXT_CLOCK_FREQ == 16000000) && (MCLK == 48000000ul) && (PLLCK=96000000ul)
   /* Configure PLL to output ~96MHz */
  *AT91C_PMC_PLLR = AT91C_CKGR_USBDIV_1 /*USB clock = PLL freq /2 */
                    | ((60-1)<<16)          /* mul = 60 */
                    | AT91C_CKGR_OUT_0  /* 80 MHZ < pll freq < 160MHz */
                    | (0x3f<<8)         /* startup time = maximum */
                    | 10;               /*div = 10 */
#else
#error "PLL div and mul values not known!"
#endif

  /* Wait till PLL locks. */
  while(!(*AT91C_PMC_SR & AT91C_PMC_LOCK))
    ;
  while(!(*AT91C_PMC_SR & AT91C_PMC_MCKRDY))
    ;

  /* Select the master clock. */
  /* Master clock is CLK source / 2 */
  *AT91C_PMC_MCKR =  AT91C_PMC_PRES_CLK_2 ;
  while(!(*AT91C_PMC_SR & AT91C_PMC_MCKRDY))
    ;

  /* CLK source is the PLL */
  *AT91C_PMC_MCKR |=  AT91C_PMC_CSS_PLL_CLK;
  while(!(*AT91C_PMC_SR & AT91C_PMC_MCKRDY))
    ;

  /* At this point the core is running at ~48 MHz. */
}

int target_init(void)
{
  /* Disable watchdog. */
  *AT91C_WDTC_WDMR=AT91C_WDTC_WDDIS;
  clock_init();
  aic_init();
  return(0);
}

int target_start(void)
{
  return(0);
}

int target_stop(void)
{
  os_int_disable();
  return(0);
}

int target_delete(void)
{
  /* empty */
  return(0);
}

/****************************** END OF FILE **********************************/

#ifndef __ARM_IS
#define __ARM_IS

#ifdef __ghs__
#pragma ghs startnomisra
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* CLZ32 : Count leading zeros */
unsigned int __CLZ32(unsigned int Rm);

/* UMULL, SMULL : High 32 bits of 32x32->64 bit multiply */
unsigned int __MULUH(unsigned int Rm, unsigned int Rs); /* unsigned */
  signed int __MULSH(  signed int Rm,   signed int Rs); /* signed */

/* UMULL, SMULL : Full 64 bits of 32x32->64 bit multiply */
unsigned long long __UMULL(unsigned int Rm, unsigned int Rs); /* unsigned */
  signed long long __SMULL(  signed int Rm,   signed int Rs); /* signed */

/* SMLAL, UMLAL : 32x32->64 multiply with 64 bit accumulate */
  signed long long __SMLAL(  signed long long Rd,   signed int Rm,   signed int Rs);
unsigned long long __UMLAL(unsigned long long Rd, unsigned int Rm, unsigned int Rs);

/* __MULUH64 : High 64 bits of 64x64->128 bit multiply sequence using UMULL */
unsigned long long __MULUH64(unsigned long long Rm, unsigned long long Rs);

/* MRS, MSR : retrieve and set the contents of the cpsr status register */
unsigned int __GETSR(void);
void __SETSR(unsigned int Rm);

/* __DIR : Disable maskable interrupts, and return a key for use with __RIR */
unsigned int __DIR(void);

/* __EIR : Enable maskable interrupts, and return a key for use with __RIR */
unsigned int __EIR(void);

/* __RIR : Restore interrupts to their state prior to a call to __DIR or __EIR */
void __RIR(unsigned int);

/* Retrieve address for return from function from link register or frame.
   The level parameter is ignored. */
void * __builtin_return_address(unsigned int level);

#ifdef __EDG__

#include <ghs_null.h>	/* for size_t */

/* stackccv/sortedstackccv: returns a pointer to the first location used for
 * the parameters of the current function */
void *__ghs_start_param(void);

/* stackccv/sortedstackccv: returns the size of the parameter space used by
 * the current function */
size_t __ghs_param_size(void);

#endif /* def __EDG__ */

#ifdef __cplusplus
}
#endif

#ifdef __ghs__
#pragma ghs endnomisra
#endif

#endif

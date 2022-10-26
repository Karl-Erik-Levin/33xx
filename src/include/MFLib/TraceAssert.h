/* TraceAssert.h - Dataton 3397 Transponder 
**   
** Trace and Assert
**
**
** Created 09-09-24	Kalle
**
** (C) Copyright Dataton AB 2009, All Rights Reserved
**
*/

#ifndef _TraceAssert_
#define _TraceAssert_

extern void 
	MyAssert(const char*),						// Fatal error message
	MySuperAssert(const char* fmt, ...),		// Fatal error message with params
	TracePrintf(const char *frmt, ...);			// Ditto, with params

#ifdef DEBUG
#if 0
	#define ASSERT(t,s)		((t) ? (void)0 : MyAssert(s))
#else
	#define ASSERT(t,s)		((t) ? (void)0 : MySuperAssert##s)
#endif
	#define FAILMSG(s)		(MyAssert(s))
	#define TRACE_PRINTF(a) TracePrintf##a		// The following must use two parens around macro arg!

	#define TRACE_MAI(x) TracePrintf##x			// main_firm.c
	#define TRACE_TRS(x) TracePrintf##x			// trspAppl.c
	#define TRACE_GDE(x) TracePrintf##x			// micAppl.c, pickupAppl.c
	#define TRACE_MIC(x) TracePrintf##x			// pcmAppl.c
	#define TRACE_STM(x) TracePrintf##x			// ls in pickupAppl.c
	#define TRACE_USB(x) TracePrintf##x			// usbAppl.c
	#define TRACE_USV(x) 						// usbVenReqHdlr.c
	#define TRACE_AUD(x) TracePrintf##x			// audMng.c & mp3Util.c
	#define TRACE_AST(x) 						// setMng.c
	#define TRACE_FMT(x) TracePrintf##x 		// fmtDrv.c
	#define TRACE_HPC(x) 						// hpcDrv.c
	#define TRACE_PWR(x) 						// pwrDrv.c
	#define TRACE_RTC(x) 						// rtcDrv.c
	#define TRACE_RAD(x)						// radMgr.c
#else
	// NoOp versions of the above macros
	#define ASSERT(t,s)		((void)0)
	#define FAILMSG(s)		((void)0)
	#define TRACE_PRINTF(a) ((void)0)

	#define TRACE_MAI(x)  		// main_firm.c
	#define TRACE_TRS(x) 		// trspAppl.c
	#define TRACE_GDE(x) 		// micAppl.c, pickupAppl.c
	#define TRACE_USB(x) 		// usbAppl.c
	#define TRACE_USV(x) 		// usbVenReqHdlr.c
	#define TRACE_AUD(x) 		// audMng.c
	#define TRACE_AST(x) 		// setMng.c
	#define TRACE_FMT(x) 		// fmtDrv.c
	#define TRACE_HPC(x) 		// hpcDrv.c
	#define TRACE_PWR(x) 		// pwrDrv.c
	#define TRACE_RTC(x) 		// rtcDrv.c
	#define TRACE_RAD(x)						// radMgr.c
#endif

#endif // _TraceAssert_


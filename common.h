/** 
 * File:   common.h
 *
 * This header includes stdint.h, int_def.h, and the processor header file as
 * well as other useful macros used to assist in writing reusable code.
 *
 * 
 *
 * @author Michael Muhlbaier
 * @{
 */

#ifndef _COMMON_H_
#define	_COMMON_H_

#include "int_def.h"

#ifdef __C32__
#include <plib.h>
#else
#if defined(__PIC24E__)
#include <p24Exxxx.h>
#elif defined(__PIC24F__)
#include <p24Fxxxx.h>
#elif defined(__PIC24H__)
#include <p24Hxxxx.h>
#endif
#endif

/** concatenate 2 strings together
 *
 * Useful for making reusable defines
 * For example, the timing module uses this macro to concatenate the letters PR
 * with the TIMING_TIMER number to make the period register define
 * @code
 * #define TIMER_PERIOD CAT2(PR,TIMING_TIMER)
 * // if TIMING_TIMER is 2 this would be equivalent to
 * #define TIMER_PERIOD PR2
 * // but using CAT2 allows the user to just specify which timer they want to
 * // use and the defines in timing.h take care of the rest automatically.
 * @endcode
 */
#define CAT2(s1,s2) CAT2B(s1,s2)
#define CAT2B(s1,s2) s1##s2

/** concatenate 3 strings together
 * 
 * Useful for making reusable defines
 * For example, the timing module uses this macro to concatenate the letter T
 * with the TIMING_TIMER number with the letters CON:
 * @code
 * #define ConfigureTimer() CAT3(T,TIMING_TIMER,CON) = 0xA010
 * // if TIMING_TIMER is 2 this would be equivalent to
 * #define ConfigureTimer() T2CON = 0xA010
 * // but using CAT3 allows the user to just specify which timer they want to
 * // use and the defines in timing.h take care of the rest automatically.
 * @endcode
 */
#define CAT3(s1,s2,s3) CAT3B(s1,s2,s3)
#define CAT3B(s1,s2,s3) s1##s2##s3

/** concatenate 4 strings together
 * 
 * Useful for making reusable defines
 * For example:
 * @code
 * #define TIMER_INTERRUPT_FLAG CAT4(TIMING_IFS,bits.T,TIMING_TIMER,IF)
 * // if TIMING_TIMER is 2 and TIMING_IFS is IFS0 this would be equivalent to
 * #define TIMER_INTERRUPT_FLAG IFS0bits.T2IF
 * @endcode
 */
#define CAT4(s1,s2,s3,s4) CAT4B(s1,s2,s3,s4)
#define CAT4B(s1,s2,s3,s4) s1##s2##s3##s4

/** concatenate 5 strings together
 * 
 * Useful for making reusable defines
 * For example:
 * @code
 * #define TIMER_INTERRUPT_FLAG CAT4(TIMING_IFS,bits.T,TIMING_TIMER,IF)
 * // if TIMING_TIMER is 2 and TIMING_IFS is IFS0 this would be equivalent to
 * #define TIMER_INTERRUPT_FLAG IFS0bits.T2IF
 * @endcode
 */
#define CAT5(s1,s2,s3,s4,s5) CAT5B(s1,s2,s3,s4,s5)
#define CAT5B(s1,s2,s3,s4,s5) s1##s2##s3##s4##s5

/** @} */
#endif	/* _COMMON_H_ */


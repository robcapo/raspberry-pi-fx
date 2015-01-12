// _TIMING_H_ is a flag used to prevent timing.h from being included more than
// once in any given source file.
#ifndef _TIMING_H_
#define _TIMING_H_

#include "system.h"
#include "common.h"

/** @file
 * @addtogroup timing Timing module
 *
 * This module provides some very basic functions to make timing easier. Several
 * other modules rely on the services provided by this module (e.g. #log and
 * #tasks).
 *
 * The defines control the hardware specific aspects of the timing module. An
 * ISR is needed to increment the time, this may need to be changed in timing.c
 * @{
 */

/** system.h would be the best place to specify which timer the timing module
 * should use (default is timer 5 on PIC24 or PIC32 if not specified)
 * @code
 * // to use timer 3 on a PIC32 add the following to system.h
 * #define TIMING_TIMER 3
 * #define TIMING_IPC IPC3
 * #define TIMING_IFS IFS0
 * #define TIMING_IEC IEC0
 * // to use timer 1 on a Tiva C add the following to system.h
 * #define TIMING_TIMER 1
 * // if using a MSP430 no definition is needed, it will use TimerA0 by default. Or:
 * #define TIMING_TIMER 1
 * @endcode
 *
 * system.h would also be the best place to define SYSTEM_CLOCK
 */
#include "system.h"

/** @def Flag to enable logging module
 *
 * Set in system.h
 */
//#define TIMING_LOGGING_ENABLE

// if logging is enabled then make sure log.h is included as well as version info
#ifdef TIMING_LOGGING_ENABLE
#include "log.h"

/** timing module version, format is 8bits major version, 8bits minor version
 * and 16 bits for the build. If any changes are made increment the build number,
 * if features are added or removed change and minor and/or major version number.
 *
 * Ignored if TIMING_LOGGING_ENABLE is not defined
 */
#define TIMING_VERSION (version_t)0x02010001UL
#endif // TIMING_LOGGING_ENABLE

#ifndef FCY
#ifdef PERIPHERAL_CLOCK
#define FCY PERIPHERAL_CLOCK
#else
#define FCY SYSTEM_CLOCK
#endif
#endif

#ifdef __MSP430G2553__
#include <msp430.h>
#ifndef TIMING_TIMER
#define TIMING_TIMER 0
#endif
#define TIMING_PRESCALE 1
#if TIMING_TIMER == 1
	#define TimerInterruptEnable() TA1CTL |= TAIE
	#define TimerClearInterruptFlag() TA1CTL &= ~0x01 // clear bit 0 which is TAIFG
	#define ConfigureTimer() TA1CTL |= TASSEL_2 | MC_1
	#define TimerPeriodSet(x) TA1CCR0 = x
	#define TimingISR(void) _Pragma("vector=TIMER0_A1_VECTOR") \
		__interrupt void _TimingISR(void)
#else
	#define TimerInterruptEnable() TA0CTL |= TAIE
	#define TimerClearInterruptFlag() TA0CTL &= ~TAIFG // clear bit 0 which is TAIFG
	#define ConfigureTimer() TA0CTL |= TASSEL_2 | MC_1
	#define TimerPeriodSet(x) TA0CCR0 = x
	#define TimingISR(void) _Pragma("vector=TIMER0_A1_VECTOR") \
		__interrupt void _TimingISR(void)
#endif
#define TimerConfigHardware()
#define SetTimerInterruptPriority(x)
#elif defined PART_TM4C123GH6PM
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
//#include "driverlib/gpio.h"
//#include "driverlib/pin_map.h"
#include "driverlib/timer.h"
void TimingISR(void);
#ifndef TIMING_TIMER
#define TIMING_TIMER 1
#endif
#define TIMING_BASE CAT3(TIMER,TIMING_TIMER,_BASE)
#define TIMING_PRESCALE 1
#define TimerConfigHardware() SysCtlPeripheralEnable(CAT2(SYSCTL_PERIPH_TIMER,TIMING_TIMER))
#define ConfigureTimer() TimerConfigure(TIMING_BASE, (TIMER_CFG_PERIODIC_UP)); \
	TimerIntRegister(TIMING_BASE, TIMER_A, TimingISR); \
	TimerEnable(TIMING_BASE, TIMER_A)
#define TimerPeriodSet(p) TimerLoadSet(TIMING_BASE, TIMER_A, p);
#define TimerClearInterruptFlag() TimerIntClear(TIMING_BASE, TIMER_TIMA_TIMEOUT)
#define TimerInterruptEnable() TimerIntEnable(TIMING_BASE, TIMER_TIMA_TIMEOUT)
#define SetTimerInterruptPriority(x)
#else
#define TimerConfigHardware()
/** Period register for the timer */
#define TIMER_PERIOD CAT2(PR,TIMING_TIMER)
#define TimerPeriodSet(x) TIMER_PERIOD = x;
/** Register for the timer */
#define TIMER CAT2(TMR,TIMING_TIMER)
/** Set interrupt priority for the timer interrupt */
#define SetTimerInterruptPriority(x) CAT4(TIMING_IPC,bits.T,TIMING_TIMER,IP) = x
/** Interrupt flag bit for the timer */
#define TIMER_INTERRUPT_FLAG CAT4(TIMING_IFS,bits.T,TIMING_TIMER,IF)
#define TimerClearInterruptFlag() TIMER_INTERRUPT_FLAG = 0
/** Interrupt enable bit for the timer */
#define TIMER_INTERRUPT_ENABLE CAT4(TIMING_IEC,bits.T,TIMING_TIMER,IE)
#define TimerInterruptEnable() TIMER_INTERRUPT_ENABLE = 1

// if FCY/1000 will overflow a 16-bit register then set the CON and Prescale
// in settings.h
#if FCY < 65535000
#define TIMING_CON 0xA000
#define TIMING_PRESCALE 1
#endif

#define ConfigureTimer() CAT3(T,TIMING_TIMER,CON) = TIMING_CON
#define TIMING_VECTOR CAT3(_TIMER_,TIMING_TIMER,_VECTOR)
#define TIMING_INTERRUPT CAT3(_T,TIMING_TIMER,Interrupt)
#endif

/** Timing period for one millisecond, 1:1 prescale */
#ifdef FPB
#define TIMER_PERIOD_MS (FPB/1000/TIMING_PRESCALE)
#else
#define TIMER_PERIOD_MS (FCY/1000/TIMING_PRESCALE)
#endif

/** max time for system time */
#define TIME_MAX 0xFFFFFFFF
/** type for timer, change this to uint16_t for better efficiency on a 8 or 16
 * bit system.
 */
#define tint_t uint32_t

typedef struct {
    uint16_t ms;
    uint8_t sec;
    uint8_t min;
    uint32_t hr;
} timestruct_t;

/** Initialize the timer to use and reset the system time to zero
 */
void TimingInit(void);
// define for backwards compatibility
#define TimerInit() TimingInit()

/** Delay a specific number of milliseconds
 *
 * DelayMs uses TimeNow() and TimeSince() to implement the delay
 *
 * @param delay specifies the number of milliseconds to delay
 *
 * @author Michael Muhlbaier (mm@spaghetti.cc)
 */
void DelayMs(tint_t delay);
void DelaySec(uint16_t delay);
void DelayMin(uint16_t delay);
void DelayHr(uint16_t delay);

/** Calculate the time since the given time
 *
 * Calculate and return the time since a given time. Typically used with
 * TimeNow(). See the example usage.
 *
 * @param time the time in milliseconds to calculate the time since
 * @return the time since the time given in milliseconds
 *
 * @code
 * int t;
 * t = TimeNow();
 * // wait for 1 second to pass
 * while(TimeSince(t) < 1000);
 * @endcode
 *
 * @see TimeNow
 */
tint_t TimeSince(tint_t time);
uint32_t TimeSinceSec(tint_t time);
uint32_t TimeSinceMin(tint_t time);
uint16_t TimeSinceHr(tint_t time);
timestruct_t TimeSinceF(tint_t time);

/** Returns the current system time in milliseconds
 *
 * @return the current system time in milliseconds
 *
 * @see TimeSince
 */
tint_t TimeNow(void);
tint_t TimeNowSec(void);
tint_t TimeNowMin(void);
uint16_t TimeNowHr(void);
timestruct_t TimeNowF(void);

/** Reset the system time
 * 
 * Reset system time (e.g. TimeNow() will return 0) and set appropriate 
 * rollover times so TimeSince will work properly.
 * 
 * @warning Timing discrepancies will occur if TimerRoll is called and not called
 * again for the next timer rollover. TimeSince() will result in a shorter time
 * than expected if the timer is allowed to naturally roll over after calling
 * TimerRoll. The difference will be MAX_TIME - the time when TimerRoll was last
 * called. This could be fixed by adding a check for the timer rolling over naturally
 * and setting rollover_time to TIME_MAX in timing.c
 */
void TimerRoll(void);

/** Set the period of the system timer
 *
 * @param period period time = FCY/2/period (or FPB/2/period)
 */
void TimerPeriod(uint16_t period);

/** @} */

#endif // _TIMING_H_

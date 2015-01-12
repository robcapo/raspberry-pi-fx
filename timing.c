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

#include "timing.h"

volatile tint_t ms_time, rollover_time;
timestruct_t total_time; // stores the total time since the last rollover

// this ISR is device specefic and might need to change
#ifdef __C32__
void __ISR(TIMING_VECTOR) TimingInterrupt(void) {
#elif defined PART_TM4C123GH6PM
void TimingISR(void) {
#elif defined __MSP430G2553__
TimingISR(void) {
#else
void __attribute__((interrupt, auto_psv)) TIMING_INTERRUPT(void) {
#endif
    // increment variables at 1/1000 of a second
    ms_time++;
    TimerClearInterruptFlag();
}

void TimingInit(void) {
//    ms_time = 4294847296; // 120 seconds from rollover
	TimerConfigHardware();
	TimerPeriodSet(TIMER_PERIOD_MS);
    //TIMER = 0;
    SetTimerInterruptPriority(4);
    TimerClearInterruptFlag();

    rollover_time = TIME_MAX;
    total_time.ms = 0;
    total_time.sec = 0;
    total_time.min = 0;
    total_time.hr = 0;

    ConfigureTimer();

    TimerInterruptEnable();
    // initialize the timing subsystem for logging ( this can probably be
    // removed unless a need for logging from within the timing system is
    // realized)
#ifdef TIMING_LOGGING_ENABLE
    LogInitSubsystem(TIMING, EVERYTHING, "timing", TIMING_VERSION);
#endif
}

tint_t TimeNow(void) {
    return ms_time;
}

tint_t TimeNowSec(void) {
    return (ms_time / 1000);
}

tint_t TimeNowMin(void) {
    return (ms_time / 60000);
}

uint16_t TimeNowHr(void) {
    return (ms_time / 3600000);
}

timestruct_t TimeNowF(void) {
    timestruct_t t;
    tint_t time;
    // format ms_time into t
    time = ms_time;
    t.hr = time / 3600000;
    time -= t.hr * 3600000;
    t.min = time / 60000;
    time -= t.min * 60000;
    t.sec = time / 1000;
    time -= t.sec * 1000;
    // add total_time to t
    t.ms = time + total_time.ms;
    if(t.ms >= 1000) { t.ms -= 1000; t.sec++; }
    t.sec += total_time.sec;
    if(t.sec >= 60) { t.sec -= 60; t.min++; }
    t.min += total_time.min;
    if(t.min >= 60) { t.min -= 60; t.hr++;  }
    t.hr += total_time.hr;
    return t;
}

tint_t TimeSince(tint_t time) {
    if (ms_time >= time) return (ms_time - time);

    // rollover has occurred
    return (ms_time + (1 + (rollover_time - time)));
}

tint_t TimeSinceSec(tint_t time) {
    return (TimeSince(time) / 1000);
}

tint_t TimeSinceMin(tint_t time) {
    return (TimeSince(time) / 60000);
}

uint16_t TimeSinceHr(tint_t time) {
    return (TimeSince(time) / 3600000);
}

timestruct_t TimeSinceF(tint_t time) {
    timestruct_t t;
    time = TimeSince(time);
    t.hr = time / 3600000;
    time -= t.hr * 3600000;
    t.min = time / 60000;
    time -= t.min * 60000;
    t.sec = time / 1000;
    time -= t.sec * 1000;
    t.ms = time;
    return t;
}

void TimerRoll(void) {
    timestruct_t temp;

    rollover_time = ms_time;
    // add rolloever time to the total_time
    temp.hr = rollover_time / (3600000);
    temp.min = (rollover_time - (temp.hr * 3600000)) / 60000;
    temp.sec = (rollover_time - (temp.hr * 3600000) - temp.min * 60000) / 1000;
    temp.ms = rollover_time - temp.hr*3600000 - temp.min*60000 - temp.sec*1000;
    total_time.ms += temp.ms;
    if(total_time.ms >= 1000) { total_time.ms -= 1000; total_time.sec++; }
    total_time.sec += temp.sec;
    if(total_time.sec >= 60) { total_time.sec -= 60; total_time.min++; }
    total_time.min += temp.min;
    if(total_time.min >= 60) { total_time.min -= 60; total_time.hr++; }
    total_time.hr += temp.hr;

    ms_time = 0;
#ifdef TIMING_LOGGING_ENABLE
    LogMsg(TIMING,IMPORTANT_MESSAGE,"Timer rollover forced at %l",rollover_time);
#endif
}

void DelayMs(tint_t delay) {
    tint_t time;
    time = TimeNow();
    while (TimeSince(time) <= delay) {
//        Nop();
    }
}

void DelaySec(uint16_t delay) {
    DelayMs( ((tint_t)delay) * 1000);
}

void DelayMin(uint16_t delay) {
    DelayMs( ((tint_t)delay) * 60000);
}

void DelayHr(uint16_t delay) {
    uint16_t i;
    for(i = 0; i < delay; i++) {
        DelayMs(3600000);
    }
}

void TimerPeriod(uint16_t period) {
	TimerPeriodSet(period);
}

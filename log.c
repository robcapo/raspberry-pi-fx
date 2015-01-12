/*
 * log.c
 *
 *  Created on: Mar 13, 2014
 *      Author: Michael
 */
#include "subsys.h"
#include "buffer_printf.h"

#define LogChar(c) Push(LOG_BUF,c)

void LogHeader(enum sys_index sys, enum priority_level level);

void LogStr(char * str, ...) {
    // variable argument list type
    va_list vars;
    // initialize the variable argument list pointer by specifying the
    // input argument immediately preceding the variable list
    va_start(vars, str);
    // use Push_vprintf to log to LOG_BUF
    Push_vprintf(LOG_BUF, str, vars);
    va_end(vars);
}

void LogMsg(enum sys_index sys, enum priority_level lev, char * str, ...) {
	// variable argument list type
	va_list vars;
	if (GetSubsystemPriority(sys) >= lev) {
		// output header
		LogHeader(sys, lev);
	    // initialize the variable argument list pointer by specifying the
	    // input argument immediately preceding the variable list
	    va_start(vars, str);
	    // use Push_vprintf to log to LOG_BUF
	    Push_vprintf(LOG_BUF, str, vars);
	    va_end(vars);
		// new line
	    LogChar('\r');
	    LogChar('\n');
	}
}

void LogHeader(enum sys_index sys, enum priority_level level) {
	char * ptr;
	// output timestamp
#ifdef LOG_FORMAT_TIMESTAMP
	timestruct_t t;
	t = TimeNowF();
	Push_uint16(LOG_BUF, t.hr);
	LogChar(':');
	if(t.min < 10) LogChar('0');
	Push_uint16(LOG_BUF, t.min);
	LogChar(':');
	if(t.sec < 10) LogChar('0');
	Push_uint16(LOG_BUF, t.sec);
	LogChar('.');
	if(t.ms < 100) LogChar('0');
	if(t.ms < 10) LogChar('0');
	Push_uint16(LOG_BUF, t.ms);
#else
	Push_uint32(LOG_BUF,GetLogTimestamp());
#endif
	LogChar('_');
	ptr = GetSubsystemName(sys);
	while (*ptr != 0) {
		LogChar(*ptr++);
	}
	LogChar('_');
	ptr = GetPriorityLevelName(level);
	while (*ptr != 0) {
		LogChar(*ptr++);
	}
	LogChar(' ');
}

/*
 * subsys.c
 *
 *  Created on: Mar 12, 2014
 *      Author: Michael
 */
#include "subsys.h"
#include <string.h>

void ReceiveChar(char c);
void ProcessMsg(void);
int16_t FlagStr2Int(char * ptr);
void LogLevels(void);
void LogSetAllLevels(enum priority_level level);
void LogVersions(void);
void LogSetGlobalLevel(enum priority_level level);
void LogSubsystem(int16_t s);

/** Subsystem struct to organize name, level, and version for each subsystem.
 *
 * Note: this struct is kept local to this file since the user should use the
 * following methods to set and update the subsystem information:
 * @see SubsystemLogPriority()
 * @see SubsystemInit()
 *
 */
typedef struct {
    char * name;
    enum priority_level log_priority;
    version_t version;
    void (*callback)(char * command); ///< callback function to process commands
} subsystem_t;

// array of subsystem information
subsystem_t subsystem[NUM_SUBSYSTEMS];

enum priority_level global_log_level;

// if tx buffer length is not defined elsewhere then default to 128
#ifndef UART_TX_BUFFER_LENGTH
#define UART_TX_BUFFER_LENGTH 128
#endif
// if rx buffer length is not defined elsewhere then default to 32
#ifndef UART_RX_BUFFER_LENGTH
#define UART_RX_BUFFER_LENGTH 32
#endif

#ifdef USE_UART0
	#ifndef UART0_BAUD
		#ifndef UART_BAUD
			// UART 0 Baud Rate is 9600 by default or UART_BAUD
			#define UART0_BAUD 9600
		#else
			#define UART0_BAUD UART_BAUD
		#endif
	#endif
	#ifndef NUM_UART0_RECEIVERS
		// Max number of receivers is the number of subsystems by default
		#define NUM_UART0_RECEIVERS NUM_SUBSYSTEMS
	#endif
	buffer_t rx0, tx0;
	char tx_buffer_array0[UART_TX_BUFFER_LENGTH];
	char rx_buffer_array0[UART_RX_BUFFER_LENGTH];
	receiver_t receivers0[NUM_UART0_RECEIVERS];
#endif
#ifdef USE_UART1
	#ifndef NUM_UART1_RECEIVERS
		// Max number of receivers is the number of subsystems by default
		#define NUM_UART1_RECEIVERS NUM_SUBSYSTEMS
	#endif
	#ifndef UART1_BAUD
		#ifndef UART_BAUD
			// UART 1 Baud Rate is 9600 by default or UART_BAUD
			#define UART1_BAUD 9600
		#else
			#define UART1_BAUD UART_BAUD
		#endif
	#endif
	buffer_t rx1, tx1;
	char tx_buffer_array1[UART_TX_BUFFER_LENGTH];
	char rx_buffer_array1[UART_RX_BUFFER_LENGTH];
	receiver_t receivers1[NUM_UART1_RECEIVERS];
#endif

// log level name to be used in the message header
char * priority_level_name[] = {
	"FORCE",
	"ERR1",
	"ERR2",
	"ERR3",
	"WNG1",
	"WNG2",
	"WNG3",
	"MSG1",
	"MSG2",
	"MSG3",
	"ALL_"
};

// simple functions to access different subsystem info
char *GetPriorityLevelName(enum priority_level level) {
	return priority_level_name[level];
}
enum priority_level GetSubsystemPriority(enum sys_index sys) {
	return subsystem[sys].log_priority;
}
char *GetSubsystemName(enum sys_index sys) {
	return (subsystem[sys].name == 0)?"uninitialized":subsystem[sys].name;
}

void SystemInit(void) {
	unsigned int i;
#ifdef USE_UART0
	BufferInit(&tx0, &tx_buffer_array0[0], UART_TX_BUFFER_LENGTH);
	BufferInit(&rx0, &rx_buffer_array0[0], UART_RX_BUFFER_LENGTH);
	for(i = 0; i < NUM_UART0_RECEIVERS; i++) receivers0[i] = 0;
	UART_Init(UART0_BASE, &rx0, &tx0, UART0_BAUD);
#endif
#ifdef USE_UART1
	BufferInit(&tx1, &tx_buffer_array1[0], UART_TX_BUFFER_LENGTH);
	BufferInit(&rx1, &rx_buffer_array1[0], UART_RX_BUFFER_LENGTH);
	for(i = 0; i < NUM_UART1_RECEIVERS; i++) receivers1[i] = 0;
	UART_Init(UART1_BASE, &rx1, &tx1, UART1_BAUD);
#endif
	// if the Task Management Module is being used then schedule the SubSystemTick to happen every ms
	TaskScheduleAdd(SubSystemTick,TASK_HIGH_PRIORITY,1,1);
}

void SubSystemTick(void) {
	char c;
	volatile unsigned int i;
#ifdef USE_UART0
	// if a char is in the receive buffer then Pop it and process it
	if(GetSize(&rx0)) {
		c = Pop(&rx0);
		// first call all registered receivers and pass the Pop'd char
		for(i = 0; i < NUM_UART0_RECEIVERS; i++) {
			if(receivers0[i]) receivers0[i](c);
			else break;
		}
		// then call the local subsys receiver which will parse commands
		ReceiveChar(c);
	}
#endif
#ifdef USE_UART1
	if(GetSize(&rx1)) {
		c = Pop(&rx1);
		for(i = 0; i < NUM_UART1_RECEIVERS; i++) {
			if(receivers1[i]) receivers1[i](c);
			else break;
		}
	}
#endif
}

void SubsystemInit(enum sys_index sys, enum priority_level level, char * name, version_t version) {
	subsystem[sys].log_priority = level;
	subsystem[sys].name = name;
	subsystem[sys].version.word = version.word;
}

void RegisterCallback(enum sys_index sys, void (*fn)(char * cmd)) {
	subsystem[sys].callback = fn;
}

#ifndef RECEIVE_MAX_LENGTH
#define RECEIVE_MAX_LENGTH 64
#endif
enum com_state { IDLE, WRITING };
#define RECEIVE_START_CHAR '$'
#define RECEIVE_STOP_CHAR '\r'
#define RECEIVE_FLAG_CHAR '-'
char receive_buffer[RECEIVE_MAX_LENGTH];

void ReceiveChar(char c) {
    //static char receive_buffer[LOG_RECEIVE_MAX_LENGTH];
    static enum com_state receive_state = IDLE;
    static int length;

    if (receive_state == IDLE) {
        if (c == RECEIVE_START_CHAR) {
            receive_state = WRITING;
            length = 0;
        }
    } else {
        receive_buffer[length] = c;
        if (c == RECEIVE_STOP_CHAR) {
            // process received message
            ProcessMsg();
            receive_state = IDLE;
        } else {
            length++;
            if (length > RECEIVE_MAX_LENGTH) receive_state = IDLE;
        }
    }
}

void ProcessMsg(void) {
    int16_t i = 0, s = -1, l = -1, g = -1;

    // scan for flags

    while (receive_buffer[i] != RECEIVE_STOP_CHAR) {
        // if the index is greater than length then break;
        if (i >= RECEIVE_MAX_LENGTH) break;
        // check for flag character '-' and increment i
        if (receive_buffer[i++] == RECEIVE_FLAG_CHAR) {
            // if a flag switch the flag character and process the value if needed
            switch (receive_buffer[i]) {
                case 'g':
                case 'G':
                    g = FlagStr2Int(&receive_buffer[++i]);
                    break;
                case 'l':
                case 'L':
                    l = FlagStr2Int(&receive_buffer[++i]);
                    break;
                case 's':
                case 'S':
                    s = FlagStr2Int(&receive_buffer[++i]);
                    break;
                default:
                    LogMsg(SYSTEM, WARNING, "Unknown flag");
                    break;
            }
        }
    }

    // compare the receive buffer to known commands and process accordingly
    // only compare the beginning of the identifier to allow short alias' to be
    // used (e.g. strncmp(&buf, "ver", 3) would match "ver" or "version"
    // if a command needs to be added that has its first 3 letters such that
    // they match below then make sure to put the new command above the matching
    // command in the if-else if logic.
    if (strncmp(&receive_buffer[0], "ver", 3) == 0) {
        // "version" or "ver" command, output version information
        LogVersions();
    } else if (strncmp(&receive_buffer[0], "lev", 3) == 0) {
        // "level" or "lev" command, if no flags then output level codes
        // if -l flag is given but not -s flag then set the global level
        // if -l and -s flags are given then set the level for the subsystem
        if (s == -1 && l == -1) LogLevels();
        else if (s == -1) {
            LogSetAllLevels((enum priority_level)l);
            LogMsg(SYSTEM, (enum priority_level)l, "<- Global log level set");
        } else if (l > -1) {
            subsystem[s].log_priority = (enum priority_level)l;
            LogMsg((enum sys_index)s, (enum priority_level)l, "<- Log level set");
        }
        // if -g flag is given the set the global log level
        if(g > -1) LogSetGlobalLevel((enum priority_level)g);
    } else if (strncmp(&receive_buffer[0], "sys", 3) == 0
            || strncmp(&receive_buffer[0], "sub", 3) == 0) {
        // "system" or "subsystem" or "sys" or "sub" command
        // output subsystem information
        // if -s flag is given only output the information for the given
        // subsystem
        if(s < 0) {
            LogMsg(SYSTEM, FORCE_LOG, "Subsystem listing:");
            for (i = 0; i < NUM_SUBSYSTEMS; i++) {
                LogSubsystem(i);
            }
        }else {
            LogSubsystem(s);
        }

    } else {
        // check if the command is the name of a subsystem, if so forward the
        // command to the subsystem
        for(s = 1; s < UNKNOWN; s++) {
            if(subsystem[s].name != 0) {
                i = 0;
                while(receive_buffer[i] == *(subsystem[s].name+i)) {
                    i++;
                    // if we get to the end of the name then we have a match, send
                    // the rest of the command to the callback function
                    if(*(subsystem[s].name+i) == 0) {
                        subsystem[s].callback(&receive_buffer[i + 1]);
                        return;
}
                }
            }
        }

#ifdef NUM_ADDRESS
        if (receive_buffer[0] >= '0' && receive_buffer[0] <= '9') TunnelMsg(&receive_buffer[0]);
        return;
#endif
        LogMsg(SYSTEM, FORCE_LOG, "Unknown command");
    }
}

int16_t FlagStr2Int(char * ptr) {
    // make sure a number is after the flag
    if (*ptr >= '0' && *ptr <= '9') {
        // can accept 0 to 99
        if (*(ptr+1) >= '0' && *(ptr+1) <= '9') {
            return (*ptr - '0')*10 + *(ptr+1) - '0';
        } else return *ptr - '0';
    } else {
        // code could be added here to allow flags to use level
        // names instead of numbers
        LogMsg(SYSTEM, WARNING, "Invalid level flag format");
        return 0;
    }
}

void LogVersions(void) {
    uint16_t i;
    char * ptr;
    version_mmb_t * v;
    LogMsg(SYSTEM, FORCE_LOG, "Subsystem versions:");
    for (i = 0; i < NUM_SUBSYSTEMS; i++) {
        ptr = subsystem[i].name;
        if (ptr) {
            v = &subsystem[i].version.v;
            LogStr("%s %u.%u.%u\r\n", ptr, v->major, v->minor, v->build);
        }
    }
}

void LogSubsystem(int16_t s) {
    char *ptr, *ptr2;

    ptr = GetSubsystemName((enum sys_index)s); // cast to sys_index not required but avoids warning on strict compilers
    if(subsystem[s].log_priority == FORCE_LOG) ptr2 = "OFF";
    else ptr2 = priority_level_name[subsystem[s].log_priority];
    LogStr("%u %s, level: %s\r\n", s, ptr, ptr2);
}

void LogLevels(void) {
    enum priority_level level;

    LogMsg(SYSTEM, FORCE_LOG, "Log level listing:");
    for (level = FORCE_LOG; level <= EVERYTHING; level++) {
        LogStr("%d %s\r\n", level, priority_level_name[level]);
    }
}

void LogSetAllLevels(enum priority_level level) {
    uint16_t i;
    for (i = 0; i < NUM_SUBSYSTEMS; i++) {
        subsystem[i].log_priority = level;
    }
}

void LogSetGlobalLevel(enum priority_level level) {
    global_log_level = level;
}

void LogSubsystemLevel(enum sys_index sys, enum priority_level level) {
    subsystem[sys].log_priority = level;
}

#ifdef USE_UART0
void RegisterReceiverUART0(receiver_t fn) {
	volatile uint16_t i;
	// save the function pointer (fn) in the first open slot in the receivers array
	for(i = 0; i < NUM_UART0_RECEIVERS; i++) {
		if(receivers0[i] == 0) {
			receivers0[i] = fn;
			break;
		}
	}
}

void UnregisterReceiverUART0(receiver_t fn) {
	volatile uint16_t i, j;
	// loop through the receivers array for the function pointer (fn)
	// when found shift remaining array items left to overwrite (fn)
	// then set the last function pointer to 0
	for(i = 0; i < NUM_UART0_RECEIVERS; i++) {
		if(receivers0[i] == fn) {
			for(j = i+1; j < NUM_UART0_RECEIVERS; j++) {
				receivers0[j-1] = receivers0[j];
			}
			receivers0[j-1] = 0;
		}
	}
}
#endif // USE_UART0

#ifdef USE_UART1
void RegisterReceiverUART1(receiver_t fn) {
	volatile uint16_t i;
	for(i = 0; i < NUM_UART1_RECEIVERS; i++) {
		if(receivers1[i] == 0) {
			receivers1[i] = fn;
			break;
		}
	}
}

void UnregisterReceiverUART1(receiver_t fn) {
	volatile uint16_t i, j;
	for(i = 0; i < NUM_UART1_RECEIVERS; i++) {
		if(receivers1[i] == fn) {
			for(j = i+1; j < NUM_UART1_RECEIVERS; j++) {
				receivers1[j-1] = receivers1[j];
			}
			receivers1[j-1] = 0;
		}
	}
}
#endif // USE_UART1

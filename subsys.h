/**
 * @file subsys.h
 *
 * @defgroup subsys Subsystem Module
 *
 *  Created on: Mar 12, 2014
 *      @author: Michael
 *
 * @version 2014.03.26 changed SystemTick to SubsystemTick so the Task Management Module can own SystemTick()
 * @{
 */

#ifndef SUBSYS_H_
#define SUBSYS_H_

#include "task.h"
#include "system.h"
#include "uart.h"
typedef void (*receiver_t)(char);

/** Version typedef to store software version of subsystem
 *
 * The version is split into three numbers:
 * major.minor.build
 * where major and minor are 0-255 and build is 0-65535
 * build should be incremented as frequently as possible (automatically on build
 * if possible with the compiler)
 */
typedef struct {
        uint16_t build;
        uint8_t minor;
        uint8_t major;
} version_mmb_t;
typedef union {
    uint32_t word;
    version_mmb_t v;
} version_t;

/** @page sys_receive SubSystem module command interface
 *
 * If bi-directional communication is available (e.g. UART) then the
 * user can interface with the SubSystem module in real time using the
 * module's command interface.
 *
 * A command can be sent to the module in the following format:
 * @code
 * $command -f0
 * @endcode
 * Where the $ indicates the start of the command, command is the name of the
 * command and -f8 is a flag with a value of 8. \n
 * Valid system flags are @c -s, @c -l and @c -g
 *
 * The commands available are:
 *
 * @c version (or @c ver) - will output a list of subsystems and their version.
 * No flags are applicable.
 *
 * @c level (or @c lev) - if no flags are given @c level will output level a
 * list of valid level codes and their names as used in the log messages. If a
 * @c -l flag is given but not a @c -s flag then the level specified with the
 * @c -l flag is set as the global level. If the @c -l and @c -s flags are given
 * then set the level specified with the @c -l flag for the subsystem specified
 * with the @c -s flag. If a @c -g flag is given it will set the global log
 * level to the specified value. The global log level will for any message from
 * any subsystem to be logged if its level is at the global level or below.
 *
 * @c subsystem (or @c sub or @c sys or @c system) - will output a list of
 * valid subsystem codes and their associated subsystem names and their current
 * log level setting as used by the log message functions. The @c -s flag can
 * be used to return only information about the specified subsystem.
 *
 * Example usage:
 * @code
 * $version
 *
 * $sub
 *
 * $level
 *
 * $level -l0
 *
 * $level -s0 -l2
 * @endcode
 *
 * In the above example:
 * - output the subsystems and their versions
 * - output the subsystems and their codes
 * - output the log levels and their codes
 * - set the global log level to 0 (OFF)
 * - set the log level for subsystem 0 (SYSTEM) is set to 2 (ERROR).
 *
 * The end result is that no messages would be logged unless they were SYSTEM
 * messages with a priority level greater than or equal to ERROR (note higher
 * priority is lower numerically).
 *
 * Additionally commands may be forwarded to the callback of compatible subsystems
 * using the following format
 * @code
 * $12 command <anything goes>
 * $MUH play
 * @endcode
 * In the first line subsystem index 12 is sent the command string starting
 * after the space. In the second line a module named MUH is sent a command
 * "play"
 */

/** @page subsys_init Subsystem module initialization
 *
 * Below are three example of how a subsystem could initialize itself.
 *
 * @code
 * // define the version
 * #define TASK_VERSION (version_t)0x01010014u
 *
 * // initialize module to log EVERYTHING and to use "task" to refer
 * // to this subsystem in output messages
 * SubsystemInit(TASK, EVERYTHING, "task", TASK_VERSION);
 *
 * // or another way to do the same thing
 *
 * #define TASK_VERSION_MAJOR 1
 * #define TASK_VERSION_MINOR 1
 * #define TASK_VERSION_BUILD 20
 *
 * version_t task_version;
 * task_version.major = TASK_VERSION_MAJOR;
 * task_version.minor = TASK_VERSION_MINOR;
 * task_version.build = TASK_VERSION_BUILD;
 *
 * SubsystemInit(TASK, EVERYTHING, "task", task_version);
 *
 * // or to do it all in one line
 * SubsystemInit(TASK, EVERYTHING, "task", (version_t)0x01010014u);
 * @endcode
 */

/////////////// ENUMERATION TYPES
/** priority Level enumeration used for log commands and tasks
 * or saved.
 *
 * The levels are:
 *  - OFF / FORCE / FORCE_LOG (use OFF when setting level, use FORCE_LOG for messages)
 *  - IMPORTANT_ERROR / HIGHEST_PRIORITY
 *  - ERROR / HIGHER_PRIORITY
 *  - VERBOSE_ERROR / HIGH_PRIORITY
 *  - IMPORTANT_WARNING / MEDIUM_HIGH_PRIORITY
 *  - WARNING / MEDIUM_PRIORITY
 *  - VERBOSE_WARNING / MEDIUM_LOW_PRIORITY
 *  - IMPORTANT_MESSAGE / LOW_PRIORITY
 *  - MESSAGE / LOWER_PRIORITY
 *  - VERBOSE_MESSAGE / LOWEST_PRIORITY
 *  - EVERYTHING (does not make sense to use for messages)
 *
 * These levels can be set for specific subsystems
 */
enum priority_level   {
	OFF = 0, FORCE = 0, FORCE_LOG = 0,
    IMPORTANT_ERROR = 1, HIGHEST_PRIORITY = 1,
    ERROR, HIGHER_PRIORITY = 2,
    VERBOSE_ERROR, HIGH_PRIORITY = 3,
    IMPORTANT_WARNING, MEDIUM_HIGH_PRIORITY = 4,
    WARNING, MEDIUM_PRIORITY = 5,
    VERBOSE_WARNING, MEDIUM_LOW_PRIORITY = 6,
    IMPORTANT_MESSAGE, LOW_PRIORITY = 7,
    MESSAGE, LOWER_PRIORITY = 8,
    VERBOSE_MESSAGE, LOWEST_PRIORITY = 9,
    EVERYTHING
};

/** @enum sys_index
 * @brief Subsystem index enumeration
 *
 * First element must be SYSTEM at index 0 and last element must be UNKNOWN
 * Should be in sequential order. It is recommended to implement this enum in
 * system.h
 */
/* copy and paste the below enum to system.h
// subsytem enumeration needed for the subsys module.
enum sys_index {
    SYSTEM = 0,
    // ADD SUBSYSTEMS BELOW //
    TIMING,
    TASK,
    // ADD SUBSYSTEMS ABOVE //
    UNKNOWN
};
*/

#ifndef LOG_BUF
#define LOG_BUF &tx0
#endif

 /** GetLogTimestamp must be defined so that it returns a integer (up to 32 bits)
  * timestamp
  */
 #define GetLogTimestamp() 0

// use Push_printf to log messages to the log buffer LOG_BUF
 /** Logs the null terminated string at the pointer (str)
  *
  * Same as LogMsg() without the header in the beginning and without the CRLF at
  * the end.
  *
  * This function is implemented using Push_vprintf. See Push_printf() for supported
  * flags/features.
  *
  * Will log the string to the buffer defined by LOG_BUF (typically tx0)
  *
  * @param str pointer to string to log
  * @param ... variable number of replacement parameters for the str string
  *
  * Example usage:
  * @code
  *   LogStr("oops I crapped my pants");
  *   LogStr("System Index %d, System Name %s.", SYSTEM, GetSubsystemName(SYSTEM));
  * @endcode
  */
void LogStr(char * str, ...);

/** Logs the message at the pointer (str) with a timestamp and subsystem name
 *
 * Before logging the message the function will check the current log level of
 * the subsystem and compare it against the level parameter to determine if the
 * message should be logged
 *
 * This function is implemented using Push_vprintf. See Push_printf() for supported
 * flags/features.
 *
 * Will log the string to the buffer defined by LOG_BUF (typically tx0)
 *
 * @param sys subsystem index
 * @param lev log level priority/severity
 * @param str pointer to message to log
 * @param ... variable number of replacement parameters for the str string
 *
 * Example usage:
 * @code
 *   LogMsg(SYSTEM, WARNING, "oops I crapped my pants");
 *   LogMsg(SYSTEM, ERROR, "System Index %d, System Name %s.", SYSTEM, GetSubsystemName(SYSTEM));
 * @endcode
 */
void LogMsg(enum sys_index sys, enum priority_level lev, char * str, ...);

/** Initialize the subsystem module
 *
 * Will initialize system resources and buffers. Currently setup to initialize used UART channels
 * and clear all receivers.
 */
void SystemInit(void);

/** Check on system resources and call appropriate functions, callbacks, or receivers
 *
 * Note: Updated to use the Task Management Module and renamed to "SubsystemTick()".
 *
 * Example usage for SystemTick() implemented by the Task Management Module:
 * @code
 * void main(void) {
 *    ...
 *    while(1) {
 *       SystemTick();
 *    }
 * }
 * @endcode
 *
 */
void SubSystemTick(void);

/** Initialize settings for a subsystem - critical for proper logging and command interface
 *
 * If a module/subsystem uses the logging it should call this function
 * with the appropriate inputs when the subsystem is initializing.
 *
 * @param sys subsystem index
 * @param lev log level priority/severity setting (will log messages at this level and
 *        below)
 * @param name pointer to name of the subsystem (recommended to make the name
 *        8 characters or less)
 * @param version software version of subsystem, see #version_t for more info
 */
void SubsystemInit(enum sys_index sys, enum priority_level lev, char * name, version_t version);

/** Register a callback function for a subsystem
 *
 * When a command is received by the logging module for the subsystem @c sys
 * the @c callback function will be executed with a pointer to the command.
 *
 * @param sys - subsystem to register the callback for (enum sys_index type)
 * @param callback - function pointer to the function to run when a command is
 * received for the subsystem. function must have no return value and a
 * character pointer input.
 *
 * @code
 * // prototype for the callback function
 * void TimingModuleCallback(char * ptr);
 * // register the callback function for the TIMING subsystem
 * RegisterCallback(TIMING, TimingModuleCallback);
 * @endcode
 */
void RegisterCallback(enum sys_index sys, void (*callback)(char * cmd));

/** Return a pointer to a string corresponding to the name of a priority level
 *
 * @param lev - priority level to get name of
 * @return - pointer to a null terminated string corresponding to the name of the priority level
 */
char *GetPriorityLevelName(enum priority_level lev);

/** Return a pointer to a string corresponding to the name of the subsystem
 *
 * The name returned is the one set by SubsystemInit()
 *
 * @param sys - priority level to get name of
 * @return - pointer to a null terminated string corresponding to the name of the subsystem
 */
char *GetSubsystemName(enum sys_index sys);

/** Get the logging priority level of a subsystem
 *
 * The priority returned is the one set by SubsystemInit()
 *
 * @param sys - subsystem index of system to get priority of
 * @return priority_level of the given subsystem sys
 */
enum priority_level GetSubsystemPriority(enum sys_index sys);

/** Register a receiver to subscribe to UART0
 *
 * Once registered the receiver function will be called with a single char
 * input every time a char is received by UART0. The receiver will be called
 * from SystemTick()
 *
 * Synonymous with RegisterReceiver() and RegisterReceiverUART()
 *
 * @param fn - function pointer to void fn(char c) type receiver
 */
void RegisterReceiverUART0(receiver_t fn);

/** Unregister a receiver from UART0
 *
 * Synonymous with UnregisterReceiver() and UnregisterReceiverUART()
 *
 * @param fn - function pointer to void fn(char c) type receiver
 */
void UnregisterReceiverUART0(receiver_t fn);

/** Register a receiver to subscribe to UART1
 *
 * Once registered the receiver function will be called with a single char
 * input every time a char is received by UART1. The receiver will be called
 * from SystemTick()
 *
 * @param fn - function pointer to void fn(char c) type receiver
 */
void RegisterReceiverUART1(receiver_t fn);

/** Unregister a receiver from UART1
 *
 * @param fn - function pointer to void fn(char c) type receiver
 */
void UnregisterReceiverUART1(receiver_t fn);
// aliases for Receiver and ReceiverUART to use UART0 by default
#define RegisterReceiver(fn) RegisterReceiverUART0(fn)
#define RegisterReceiverUART(fn) RegisterReceiverUART0(fn)
#define UnregisterReceiver(fn) UnregisterReceiverUART0(fn)
#define UnregisterReceiverUART(fn) UnregisterReceiverUART0(fn)

/** The number of subsystems that the logging module knows about including
 * SYSTEM and UNKNOWN which will always be the first and last subsystem
 * indices
 */
#define NUM_SUBSYSTEMS (UNKNOWN - SYSTEM + 1)

// global system buffers
#ifdef USE_UART0
extern buffer_t rx0, tx0;
#endif
#ifdef USE_UART1
extern buffer_t rx1, tx1;
#endif
/** @}*/
#endif /* SUBSYS_H_ */

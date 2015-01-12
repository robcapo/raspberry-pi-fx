/* Edited by Rob Capo on Apr 9, 2014
 * Added Functions for task manager with function inputs.
 */
// _TASK_H_ is a flag used to prevent task.h from being included more than
// once in any given source file.
#ifndef _TASK_H_
#define _TASK_H_

// make sure stdint.h is included so we have defines for integer types
#include "common.h"

// make sure timing.h is included
#include "timing.h"

/** @file
 * @addtogroup task Task Management Module
 *
 * @brief The Task Management Module implements a task queue and a task schedule.
 *
 * The task queue is a double linked list of tasks in order of their priority.
 * If two tasks have the same priority then the task added to the queue first
 * will be ahead of the other task. Each time SystemTick() is called the first
 * task in the queue will execute. If the task is periodic it will be added
 * back to the task schedule once it is ran.
 *
 * The task schedule is a double linked list of tasks in order of when they are
 * scheduled to run. If two tasks are scheduled for the same time the one with
 * the higher priority will run first. Each task scheduled has a timestamp which
 * indicates when it should run as well as a period for how long after that time
 * until it runs again. When SystemTick() is called it will check to if any
 * scheduled tasks are due to run and add them to the task queue when they are
 * due.
 *
 * @version 10/4/2012
 * Remove functions were not interrupt safe -> Fixed
 *
 * @version 2/18/2014
 * Fixed bug in linking multiple tasks at once and/or scheduling multiple tasks
 * at once. Added extra checks to ensure task index is in range every time the
 * task array is accessed.
 *
 * @version 3/26/2014
 * Added compatibility with Tiva C and MSP430. Moved processor specific defines
 * from .c file to the end of the .h file. Changed a uint16_t to int16_t in IsTaskScheduled().
 * Also  removed some unreachable code
 *
 * @warning on MSP430 or Tiva C if interrupts are disabled calling TaskQueueAdd()
 * or TaskScheduleAdd() will "disable" then enable interrupts thus changing the
 * state. Consider updating code to handle this case.
 * @{
 */

/** Flag to enable logging module
 */
//#define TASK_LOGGING_ENABLE

// if logging is enabled then make sure log.h is included as well as version info
#ifdef TASK_LOGGING_ENABLE
#ifndef _LOG_H_
#include "log.h"
#endif // _LOG_H_

/** task module version, format is 8bits major version, 8bits minor version
 * and 16 bits for the build. If any changes are made increment the build number,
 * if features are added or removed change and minor and/or major version number.
 */
#define TASKS_VERSION (version_t)0x00010000u
#endif // TASK_LOGGING_ENABLE

/** The number of tasks that can be added to the task queue or task schedule
 *
 * Note: the task queue and task schedule use the same array of tasks in
 * implementing the two double linked lists. If the array becomes full the
 * module will either drop the lowest priority task in the queue or the task
 * that is to be added, whichever has the lowest priority.
 */
#ifndef MAX_TASK_LENGTH
#define MAX_TASK_LENGTH 20
#endif

/** named priority levels, priority levels can exceed those defined if needed
 *
 * CAUTION: do not use 0xFFFF (-1) as a priority value, this value is
 * reserved to indicate when a task is not used.
 */
#define TASK_DONE -1
#define TASK_NO_PRIORITY 0
#define TASK_LOWEST_PRIORITY 1
#define TASK_LOWER_PRIORITY 2
#define TASK_LOW_PRIORITY 3
#define TASK_MEDIUM_PRIORITY 4
#define TASK_HIGH_PRIORITY 5
#define TASK_HIGHER_PRIORITY 6
#define TASK_HIGHEST_PRIORITY 7

/** Opional feature to increase the priority level of tasks that have been in
 * the task queue for too long. If set the tasks priority level will be incremented
 * every TASK_BUMP_PRIORITY_TIME ms.
 */
#define TASK_BUMP_PRIORITY_TIME 500

#define SERVICE_TASKS_PERIOD 100
#define CHECK_PERM_TASKS_PERIOD 1000

/** Initialize Task Management Module
 *
 * Initializes the task management module for operation.
 */
void TaskInit(void);

/** Shutdown Task Managment Module
 * 
 * TaskShutdown() turns the task management module off. This also elminates all
 * tasks from queue and schedule. This function waits up to 1s to clear the task
 * queue.
 *
 * @author David Calhoun
 * @author Tony Samaritano
 * @author Aaron Johnson
 *
 * @see TerminateAllTasks()
 *
 */
void TaskShutdown(void);

/** Terminates Current Queue of tasks
 *
 * TerminateAllTasks() removes all tasks from the queue and schedualer
 *
 * @author David Calhoun
 * @author Tony Samaritano
 * @author Aaron Johnson
 *
 * @see RemoveTaskFromQueue()
 * @see RemoveTaskFromSchedule()
 */
void TerminateAllTasks(void);

/**
 * @brief Adds a function to the task queue.
 *
 * TaskQueueAdd() adds a task to be run by the task mananagement system to the queue.
 * Use this function to add the function to be run and the function's priority
 *
 * @author David Calhoun
 * @author Tony Samaritano
 * @author Aaron Johnson
 * @author Michael Muhlbaier
 *
 * @param fn Function Pointer - must have no parameters and no return value.
 * @param priority Tasks's priority - higher value is higher priority
 */
void TaskQueueAdd(void(*fn)(void), int16_t priority);

/** Add a function to the task queue.
 *
 * @param fn Pointer to function to run
 * @param input Pointer to function's input
 * @param priority Priority of task
 */
void TaskInputQueueAdd(void(*fn)(void *), void *input, int16_t priority);

/**
 * @brief Adds task to be scheduled for execution
 *
 * This function adds task that are schedule for later time
 * priority is ignore and tasks are order stictly by their time (FIFO)
 * the timestamp should be determined based on (TimeNow() + delay)
 * (modification) count is included to allow for diminishing scheduled tasks
 *
 * Note: tasks are not guaranteed to run at the exact time specified by delay
 * and period. Timing will depend on how often SystemTick() is called and how
 * high the priority of the task is relative to other tasks in the queue. In
 * properly designed systems SystemTick() should be called frequently enough
 * to keep the number of tasks in the queue low so scheduled tasks are run on
 * time.
 *
 * @author Aaron Johnson
 * @author David Calhoun
 * @author Tony Samaritano
 * @author Ryan Lee
 *
 * @param fn Function Pointer - must have no parameters and no return value.
 * @param priority Tasks's priority - higher value is higher priority
 * @param delay Delay before the task is first run
 * @param period Period of how often the task is run (0 no rescheduling)
 * @param count Count the number of times to be scheduled (-1 infinite)
 */
void TaskScheduleAddCount(void(*fn)(void), int16_t priority,
        tint_t delay, tint_t period, int16_t count);

/** Add a function with an input to the task manager
 *
 * @param fn Pointer to function to run
 * @param input Pointer to function's input
 * @param priority Priority of task
 * @param delay Delay before task is run
 * @param period Period of how often task is run
 * @param count Number of times to be rescheduled
 */
void TaskInputScheduleAddCount(void(*fn)(void *), void *input, int16_t priority, tint_t delay, tint_t period, int16_t count);

/**
 * @brief Adds task to be scheduled for execution (with implied infinite
 * scheduling if period is defined: count = 1)
 *
 * Note: tasks are not guaranteed to run at the exact time specified by delay
 * and period. Timing will depend on how often SystemTick() is called and how
 * high the priority of the task is relative to other tasks in the queue. In
 * properly designed systems SystemTick() should be called frequently enough
 * to keep the number of tasks in the queue low so scheduled tasks are run on
 * time.
 *
 * @author Aaron Johnson
 * @author David Calhoun
 * @author Tony Samaritano
 * @author Ryan Lee
 * @author Michael Muhlbaier
 *
 * @param fn Function Pointer - must have no parameters and no return value.
 * @param priority Tasks's priority - higher value is higher priority
 * @param delay Delay before the task is first run
 * @param period Period of how often the task is run (0 no rescheduling)
 */
void TaskScheduleAdd(void(*fn)(void), int16_t priority,
        tint_t delay, tint_t period);

/** Add a function with an input to the task manager
 *
 * @param fn Pointer to function to run
 * @param input Pointer to function's input
 * @param priority Priority of task
 * @param delay Delay before task is run
 * @param period Period of how often the task is run
 */
void TaskInputScheduleAdd(void(*fn)(void *), void *input, int16_t priority, tint_t delay, tint_t period);

/** reschedule a existing task or add the task to the schedule if it exists
 * 
 * NOT INTERRUPT SAFE
 * 
 * @param fn Function Pointer - must have no parameters and no return value.
 * @param priority Tasks's priority - higher value is higher priority
 * @param delay Delay before the task is first run
 * @param period Period of how often the task is run
 */
void TaskReSchedule(void(*fn)(void), int16_t priority, tint_t delay, tint_t period);

/** Reschedule a task that accepts an input
 *
 * @param fn Pointer to function
 * @param input Pointer to function's input
 * @param priority Priority of function
 * @param delay Delay before task is run
 * @param period Period of how often the task is run
 */
void TaskInputReSchedule(void(*fn)(void *), void *input, int16_t priority, tint_t delay, tint_t period);

/**
 * @brief Removes Task
 *
 * RemoveTask() loops through the entire task management queue and schedual to
 * remove that task from the task management system.
 *
 * @author Aaron Johnson
 * @author David Calhoun
 * @author Tony Samaritano
 *
 * @param fn The task (function) to be removed
 */
void RemoveTask(void(*fn)(void));

/** Remove task that accepts an input from the task manager
 *
 * @param fn Pointer to function
 * @param input Pointer to function's input
 */
void RemoveTaskInput(void(*fn)(void *), void *input);

/**
 * @brief Removes Task From Queue
 *
 * RemoveTaskFromQueue() removes a specific task from the queue.
 *
 * @author Aaron Johnson
 * @author David Calhoun
 * @author Tony Samaritano
 * @author Michael Muhlbaier
 *
 * @param fn The task (function) to be removed
 */
void RemoveTaskFromQueue(void(*fn)(void));

/** Remove task that accepts an input from the queue
 *
 * @param fn Pointer to function
 * @param input Pointer to input to the function
 */
void RemoveTaskInputFromQueue(void(*fn)(void *), void *input);

/**
 * @brief Removes Task From Schedule
 *
 * RemoveTaskFromSchedule() removes a specific task from the schedule.
 *
 * @param fn The task (function) to be removed
 */
void RemoveTaskFromSchedule(void(*fn)(void));

/** Remove function that accepts input from schedule
 *
 * @param fn Pointer to function
 * @param input Pointer to input
 */
void RemoveTaskInputFromSchedule(void(*fn)(void *), void *input);

/**
 * @brief Change Task's Priority
 *
 * ChangePriority() changes a specific task's priority to the specified level.
 *
 * Note: if two tasks in the queue or schedule have the same function pointer
 * then both tasks will have their priority value changed
 *
 * @author Aaron Johnson
 * @author David Calhoun
 * @author Tony Samaritano
 *
 * @param fn Task whose priority is to be changed
 * @param priority New priority level
 */
void ChangePriority(void(*fn)(void), int16_t priority);

/** Change the priority of a function with an input
 *
 * @param fn Pointer to function
 * @param input Pointer to function's input
 * @param priority New priority of function
 */
void ChangePriorityInput(void(*fn)(void *), void *input, int16_t priority);

/** Run the first task in the task queue and check if any tasks are due to run
 * in the schedule
 *
 * Run the first task in the task queue provided one exists. If the task ran has
 * a period value then it will be moved to the task schedule instead of removed.
 *
 * Check if any of the scheduled tasks are due to run. If so move any tasks
 * whose run time is at or before the current time to the task queue where they
 * will be ordered by priority and executed accordingly.
 */
void SystemTick(void);

/**
 * @brief Rolls the task management timer.
 *
 * RollTimer() checks the time value for each task in the array and rolls the
 * timer if any of the tasks are scheduled past the timer rollover point.
 *
 * Note: this function is not normally needed by the user as timer rollover is
 * automatically handled by the module. Only use this function if you want to
 * force the time to 0.
 *
 * @author Tony Samaritano
 * @author Aaron Johnson
 * @author David Calhoun
 *
 */
void RollTimer(void);

/** Wait a set number of milliseconds and run queued or shceduled tasks while
 * waiting
 *
 * WaitMs is similar to DelayMs from Timer Module, exception being that it
 * repeatedly calls SystemTick() to allow functions to be run from the Queue
 * and the Schedule to push tasks into the Queue.
 *
 * @warning not safe to be called from any function that could be called by
 * SystemTick() - suggest using a dedicated mutex/flag and only use WaitMs() in
 * main or functions called by main (not through SystemTick()).
 *
 * @param wait  time amount for the wait
 */
void WaitMs(tint_t wait);

/** Checks if a task is scheduled or queued
 * 
 * @param fn function to look for
 * @return 1 if task is scheduled or queued, 0 if not
 */
uint8_t IsTaskScheduled(void(*fn)(void));
uint8_t IsTaskInputScheduled(void(*fn)(void *), void *input);

// device specific code relating to device reset, enable, and disable of global interrupts
#ifndef Reset
#ifdef __C32__
#define Reset() SoftReset()
#elif defined __MSP430G2553__
#define Reset() WDTCTL = 0
#elif defined PART_TM4C123GH6PM
#define Reset() __asm("    .global _c_int00\n    b.w     _c_int00");
#else
#define Reset()   {__asm__ volatile ("reset");}
#endif
#endif
#ifdef __C32__
#include <plib.h>
#define DisableInterrupts() INTDisableInterrupts()
#define EnableInterrupts(x) INTRestoreInterrupts(x)
#elif defined __MSP430G2553__
#define DisableInterrupts() 1; __bic_SR_register(GIE)
#define EnableInterrupts(x) __bis_SR_register(GIE)
#elif defined PART_TM4C123GH6PM
#include "driverlib/interrupt.h"
#define DisableInterrupts() IntMasterDisable()
#define EnableInterrupts(x) IntMasterEnable()
#else
#if defined(__PIC24E__)
#include <p24Exxxx.h>
#elif defined(__PIC24F__)
#include <p24Fxxxx.h>
#elif defined(__PIC24H__)
#include <p24Hxxxx.h>
#endif
#define DisableInterrupts() SRbits.IPL; SRbits.IPL = 7
#define EnableInterrupts(x) SRbits.IPL = x;
#endif

/** @}*/

#endif // _TASK_H_

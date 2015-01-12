/* Edited by Rob Capo on Apr 9, 2014
 * Added implementations of task manager functions with function inputs
 */
#include "task.h"
#ifdef PERMANENT_TASKS
#include "permanent_tasks.h"
#endif
//#define DEBUGGING

// set DEBUGGING in a project specific file or as a compiler macro
#ifndef DEBUGGING
// used for debugging, set to return when not debugging
#define RETURN_OR_STOP return
// used for debugging, set to Reset() when not debugging
#define RESET_OR_STOP Reset()
#else
// used for debugging, set to return when not debugging
#define RETURN_OR_STOP while(1)
// used for debugging, set to Reset() when not debugging
#define RESET_OR_STOP while(1)
#endif

// allow the user to define a default task that will fill the function pointers
// that are not in use. This allows for more advanced error handling or debugging
// if not defined then make it a while(1) loop if debugging, nothing otherwise
#ifndef DEFAULT_TASK
void DefaultTask(void) {
#ifdef DEBUGGING
	while(1);
#endif
}

void DefaultInputTask(void *input) {
#ifdef DEBUGGING
	while(1);
#endif
}
#define DEFAULT_TASK DefaultTask
#define DEFAULT_INPUT_TASK DefaultInputTask
#endif

// next value to indicate that a task is waiting to be linked in the queue
#define TASK_LINK_QUEUE -2
// count value to indicate that a task is waiting to be removed from the queue
#define TASK_UNLINK_QUEUE -6
// next value to indicate that a task is waiting to be linked in the schedule
#define TASK_LINK_SCHEDULE -3
// count value to indicate that a task is waiting to be removed from the schedule
#define TASK_UNLINK_SCHEDULE -5

// task structure to hold function pointer, priority, time, and period as well
// as next and previous to implement a double linked list
typedef struct {
    void (*void_fn)(void); // void function to run

    void (*fn)(void *); 	// function with input to run
    void *input; 		// Pointer to argument to input to fn

    int16_t next; 		// the next task to execute (-1 if this is the last task)

    int16_t previous; 	// the previous task to execute (-1 if the first task)

    int16_t priority; 	// priority of the task (higher value is higher priority
    					// 0xFFFF is reserved to indicate that the task is done

    tint_t time; 		// the time the task was added to the queue
                 	 	// or the time the task is scheduled to run
    tint_t period; 		// the period for repeating the task if it is to be repeated

    int16_t count;  	// the number of times the taks should be repeated
                    	// (-1) for an infinitely repeated task
} task_t;

typedef struct {
    task_t task[MAX_TASK_LENGTH]; // the task array
    int16_t first; // the first task to be executed (-1 is none)
    int16_t last; // the last task to be executed (-1 is none)
    int16_t available; // next available slot where a task can be loaded
    int16_t length; // current length of the queue
    int16_t to_link; // index of task that needs to be linked into the queue*
    int16_t max_length; // max length is only used to store the max length of
                        // the queue, it has no function besides diagnostics
    int16_t sch_first; // the first task in the schedule
    int16_t sch_last; // the last task in the schedule
    int16_t sch_length; // current length of the schedule
    int16_t sch_to_link; // index of task to be linked into the schedule*
} task_queue_t;
// * to_link indecies are TAsK_DONE if nothing needs to be linked and 
// TASK_LINK_QUEUE or TASK_LINK_SCHEDULE if more than two tasks need to be
// linked

task_queue_t tasks;

// QueueTask will take a task and link it into the double linked task queue
// according to the priority of the task such that the first in the list will 
// have the highest priority and the last in the list will have the lowest 
// priority
static void QueueTask(int16_t index);

// UnQueueTask will unlink a task from the double linked task queue
static void UnQueueTask(int16_t this_task);

// ScheduleTask will take a task and link it into the double linked schedule
// according to the next time the task is scheduled to run such that the first
// in the list will have the soonest time and the last in the list will have
// the furthest time
void ScheduleTask(int16_t index);

// UnScheduleTask will unlink a task from the double linked task schedule
void UnScheduleTask(int16_t this_task);

// GetAvailableTask sets tasks.available to an available task slot in the task
// array. If no slot is available it will remove the last task in the task queue
// if its priority is lower than priority. Otherwise it will set available = -1
// and whatever function is seeking to add the task will have to abort.
void GetAvailableTask(int16_t priority);

// a service routine to make sure no tasks need to be linked into the queue
// or schedule and to increase the priority of any tasks that have been in the
// queue too long
void ServiceTasks(void);

// make sure perminantly scheduled tasks exist
void CheckPermSchedule(void);

// initialize the double linked lists for the task queue and the schedule and
// register the task module with the logging module
void TaskInit(void) {
    // terminate all tasks and initialize double linked lists to 0 length
    TerminateAllTasks();

    // register the TASK module with the logging module and set the log level
#ifdef TASK_LOGGING_ENABLE
    LogInitSubsystem(TASK, EVERYTHING, "task", TASKS_VERSION);
#endif
    // if permanent tasks are enabled then call CheckPermSchedule to add the
    // permanent tasks to the schedule
#ifdef PERMANENT_TASKS
    CheckPermSchedule();
#endif
}

// wait up to 1 second for the task queue to clear and then reset the task queue
// and schedule (either after 1 second or when the task queue is empty)
void TaskShutdown(void) {
    tint_t time;
    // get the current time
    time = TimeNow();

    // execute tasks for up to 1 second
    while(TimeSince(time)<1000) {
        // call SystemTick()
        SystemTick();
        // if there are no tasks left then break
        if(tasks.length == 0) break;
    }

    // terminate all tasks and initialize double linked lists to 0 length
    TerminateAllTasks();
}

// reset the task queue and schedule double linked lists and set all tasks to
// inactive (TASK_DONE)
void TerminateAllTasks(void) {
    int16_t i;

    // set first and last for both lists to -1
    tasks.first = -1;
    tasks.last = -1;
    tasks.sch_first = -1;
    tasks.sch_last = -1;
    // initialize available to the first task slot
    tasks.available = 0;
    // set the lengths to 0
    tasks.length = 0;
    tasks.sch_length = 0;
    // set max_length to 0
    // note: currently max_length is only used for diagnostics
    tasks.max_length = 0;

    // clear all tasks (set priority = TASK_DONE)
    for (i = 0; i < MAX_TASK_LENGTH; i++) {
        tasks.task[i].priority = TASK_DONE;
        tasks.task[i].void_fn = DEFAULT_TASK;
        tasks.task[i].fn = DEFAULT_INPUT_TASK;
        tasks.task[i].input = 0;
    }
}

void TaskQueueAdd(void(*fn)(void), int16_t priority) {
    int16_t i;
    task_t * task_ptr;
    // make sure next task is set, if not find the next available slot
    if (tasks.available < 0) {
        // get the next available task
        GetAvailableTask(priority);
    }

    // needs to be un-interruptable to avoid an interrupt firing at this spot
    // and adding a task to the same index
    unsigned int intStatus;
    intStatus = DisableInterrupts();

    i = tasks.available;
    // if no task is available then abort
    if(i < 0 || i >= MAX_TASK_LENGTH) {
    	EnableInterrupts(intStatus);
        RETURN_OR_STOP;
    }
    task_ptr = &tasks.task[i];
    // tasks.available is no longer available so set it to -1
    tasks.available = -1;
    // load the function, timestamp, and priority and set period to 0
    task_ptr->priority = priority;
    EnableInterrupts(intStatus);

    task_ptr->void_fn = fn;
    task_ptr->time = TimeNow();
    task_ptr->period = 0;

    // If there is no task waiting to be linked into the queue then set to_link
    // to this task, otherwise set to_link to TASK_LINK_QUEUE
    // at the next SystemTick() the to_link task will be linked or if to_link
    // is TASK_LINK_QUEUE then all tasks will be checked if they are waiting to
    // be linked
    if(tasks.to_link == TASK_DONE) tasks.to_link = i;
    else tasks.to_link = TASK_LINK_QUEUE;
    task_ptr->next = TASK_LINK_QUEUE;
}

void TaskInputQueueAdd(void(*fn)(void *), void *input, int16_t priority) {
    int16_t i;
    task_t * task_ptr;
    // make sure next task is set, if not find the next available slot
    if (tasks.available < 0) {
        // get the next available task
        GetAvailableTask(priority);
    }

    // needs to be un-interruptable to avoid an interrupt firing at this spot
    // and adding a task to the same index
    unsigned int intStatus;
    intStatus = DisableInterrupts();

    i = tasks.available;
    // if no task is available then abort
    if(i < 0 || i >= MAX_TASK_LENGTH) {
    	EnableInterrupts(intStatus);
        RETURN_OR_STOP;
    }
    task_ptr = &tasks.task[i];
    // tasks.available is no longer available so set it to -1
    tasks.available = -1;
    // load the function, timestamp, and priority and set period to 0
    task_ptr->priority = priority;
    EnableInterrupts(intStatus);

    task_ptr->fn = fn;
    task_ptr->input = input;
    task_ptr->time = TimeNow();
    task_ptr->period = 0;

    // If there is no task waiting to be linked into the queue then set to_link
    // to this task, otherwise set to_link to TASK_LINK_QUEUE
    // at the next SystemTick() the to_link task will be linked or if to_link
    // is TASK_LINK_QUEUE then all tasks will be checked if they are waiting to
    // be linked
    if(tasks.to_link == TASK_DONE) tasks.to_link = i;
    else tasks.to_link = TASK_LINK_QUEUE;
    task_ptr->next = TASK_LINK_QUEUE;
}

// QueueTask will take a task and link it into the double linked task queue
// according to the priority of the task such that the first in the list will
// have the highest priority and the last in the list will have the lowest
// priority
void QueueTask(int16_t index) {
    int16_t i;
    task_t * this_task;
    task_t * current_task;
    if(index < 0 || index >= MAX_TASK_LENGTH) RESET_OR_STOP;
    this_task = &tasks.task[index];
    if(this_task->priority == TASK_DONE) return;

    if(this_task->next < -1) this_task->next = -1;
    // check if there are no tasks in the queue
    if (tasks.length == 0 || tasks.first < 0 || tasks.last < 0) {
        tasks.length = 0;
        // set first and last to this task and set next and previous to -1
        tasks.first = index;
        tasks.last = index;
        this_task->next = -1;
        this_task->previous = -1;
    }
    // otherwise find where to put this task and re-link the list
    else {
        // scan the task queue to find where to insert this task
        // start at the last task in the list and work forward
        i = tasks.last;
        // infinite loop (we will break out of it though)
        while (1) {
            if(i < 0 || i >= MAX_TASK_LENGTH) RESET_OR_STOP;
            // if this task is of greater priority than the current one (i)
            current_task = &tasks.task[i];
            if (this_task->priority > current_task->priority) {
                // if there is no previous task then this task becomes the first
                // otherwise set the current task to the current task's previous
                // and continue
                if (current_task->previous < 0) {
                    // set first task to this one
                    tasks.first = index;
                    // set the current task's previous to this task
                    current_task->previous = index;
                    // clear this task's previous (set to -1)
                    this_task->previous = -1;
                    // set this task's next to the current one (i)
                    this_task->next = i;
                    break;
                }
                // otherwise go to the previous task (e.g. traverse the list)
                else {
                    // set the current task (i) to the current task's previous
                    i = current_task->previous;
                    // continue the scan
                    continue;
                }
            // if the priority of this task is not greater than the current task
            // then insert this task after the current task
            } else {
                // set this task's next to the current task's next
                this_task->next = current_task->next;
                // set this task's previous to the current task (i)
                this_task->previous = i;
                // set the current task's next to this task
                current_task->next = index;
                // if the current task was the last task then update last to this task
                if (this_task->next < 0) tasks.last = index;
                else if(this_task->next < MAX_TASK_LENGTH){
                // set the next tasks previous to this task
                tasks.task[this_task->next].previous = index;
                }
                break;
            }
        }
    }
    // increment the number of tasks in the task queue
    tasks.length++;
}

void TaskScheduleAddCount(void(*fn)(void), int16_t priority, tint_t delay, tint_t period, int16_t count) {
    task_t * task_ptr;
    // make sure next task is set, if not find the next available slot
    if (tasks.available < 0) {
        // get the next available task
        GetAvailableTask(priority);
    }

    // needs to be un-interruptable to avoid an interrupt firing at this spot
    // and adding a task to the same index
    unsigned int intStatus;
    intStatus = DisableInterrupts();
    // if no task is available then abort
    if(tasks.available < 0 || tasks.available >= MAX_TASK_LENGTH) {
    	EnableInterrupts(intStatus);
        RESET_OR_STOP;
    }
    task_ptr = &tasks.task[tasks.available];
    // schedule the task
    if(tasks.sch_to_link == TASK_DONE) tasks.sch_to_link = tasks.available;
    else tasks.sch_to_link = TASK_LINK_SCHEDULE;

    // tasks.available is no longer available so set it to -1
    tasks.available = -1;

    // load the function, priority, next time to run, and period
    task_ptr->priority = priority;
    EnableInterrupts(intStatus);
    task_ptr->void_fn = fn;
    task_ptr->time = TimeNow() + delay;
    task_ptr->period = period;
    task_ptr->count = count;
    task_ptr->next = TASK_LINK_SCHEDULE;

    // get the next available task slot so its ready (this is not required but
    // it will allow TaskQueueAdd() to run faster)
    // use TASK_NO_PRIORITY as the priority to ensure that we don't bump a task
    // off the queue if there are no available task slots
    GetAvailableTask(TASK_NO_PRIORITY);
}


void TaskInputScheduleAddCount(void(*fn)(void *), void *input, int16_t priority, tint_t delay, tint_t period, int16_t count) {
    task_t * task_ptr;
    // make sure next task is set, if not find the next available slot
    if (tasks.available < 0) {
        // get the next available task
        GetAvailableTask(priority);
    }

    // needs to be un-interruptable to avoid an interrupt firing at this spot
    // and adding a task to the same index
    unsigned int intStatus;
    intStatus = DisableInterrupts();
    // if no task is available then abort
    if(tasks.available < 0 || tasks.available >= MAX_TASK_LENGTH) {
    	EnableInterrupts(intStatus);
        RESET_OR_STOP;
    }
    task_ptr = &tasks.task[tasks.available];
    // schedule the task
    if(tasks.sch_to_link == TASK_DONE) tasks.sch_to_link = tasks.available;
    else tasks.sch_to_link = TASK_LINK_SCHEDULE;

    // tasks.available is no longer available so set it to -1
    tasks.available = -1;

    // load the function, priority, next time to run, and period
    task_ptr->priority = priority;
    EnableInterrupts(intStatus);
    task_ptr->fn = fn;
    task_ptr->input = input;
    task_ptr->time = TimeNow() + delay;
    task_ptr->period = period;
    task_ptr->count = count;
    task_ptr->next = TASK_LINK_SCHEDULE;

    // get the next available task slot so its ready (this is not required but
    // it will allow TaskQueueAdd() to run faster)
    // use TASK_NO_PRIORITY as the priority to ensure that we don't bump a task
    // off the queue if there are no available task slots
    GetAvailableTask(TASK_NO_PRIORITY);
}
void TaskScheduleAdd(void(*fn)(void), int16_t priority, tint_t delay, tint_t period) {
    TaskScheduleAddCount(fn,priority,delay,period,-1);
}

void TaskInputScheduleAdd(void(*fn)(void *), void *input, int16_t priority, tint_t delay, tint_t period) {
    TaskInputScheduleAddCount(fn, input, priority, delay, period, -1);
}

// NOT INTERRUPT SAFE
void TaskReSchedule(void(*fn)(void), int16_t priority, tint_t delay, tint_t period) {
    int16_t i;
    uint8_t found = 0;
    task_t * task_ptr;
//    RemoveTask(fn);
//    TaskScheduleAdd(fn, priority, delay, period);
//    if(found == 0) return;
    // check if the task is in the queue, if so make sure it is set not to be scheduled
    i = tasks.first;
    while(i >= 0) {
        if(i > MAX_TASK_LENGTH) break;
        task_ptr = &tasks.task[i];
        if(task_ptr->void_fn == fn) {
            task_ptr->period = 0;
            if(i != tasks.first) UnQueueTask(i);
        }
        i = task_ptr->next;
    }
    // check if the task is in the schedule and change the time to run
    i = tasks.sch_first;
    while(i >= 0) {
        if(i > MAX_TASK_LENGTH) break;
        task_ptr = &tasks.task[i];
        if(task_ptr->void_fn == fn) {
            if(found == 0) {
                found = 1;
                // remove the task from the schedule:
                UnScheduleTask(i);
                task_ptr->priority = priority;
                task_ptr->time = TimeNow() + delay;
                task_ptr->period = period;
                task_ptr->count = -1;
                ScheduleTask(i);
            }else {
                UnScheduleTask(i);
            }
        }
        i = task_ptr->next;
    }
    // otherwise add the task to the schedule
    if(found == 0) TaskScheduleAdd(fn, priority, delay, period);
}


// NOT INTERRUPT SAFE
void TaskInputReSchedule(void(*fn)(void *), void *input, int16_t priority, tint_t delay, tint_t period) {
    int16_t i;
    uint8_t found = 0;
    task_t * task_ptr;
//    RemoveTask(fn);
//    TaskScheduleAdd(fn, priority, delay, period);
//    if(found == 0) return;
    // check if the task is in the queue, if so make sure it is set not to be scheduled
    i = tasks.first;
    while(i >= 0) {
        if(i > MAX_TASK_LENGTH) break;
        task_ptr = &tasks.task[i];
        if(task_ptr->fn == fn && task_ptr->input == input) {
            task_ptr->period = 0;
            if(i != tasks.first) UnQueueTask(i);
        }
        i = task_ptr->next;
    }
    // check if the task is in the schedule and change the time to run
    i = tasks.sch_first;
    while(i >= 0) {
        if(i > MAX_TASK_LENGTH) break;
        task_ptr = &tasks.task[i];
        if(task_ptr->fn == fn && task_ptr->input == input) {
            if(found == 0) {
                found = 1;
                // remove the task from the schedule:
                UnScheduleTask(i);
                task_ptr->priority = priority;
                task_ptr->time = TimeNow() + delay;
                task_ptr->period = period;
                task_ptr->count = -1;
                ScheduleTask(i);
            }else {
                UnScheduleTask(i);
            }
        }
        i = task_ptr->next;
    }
    // otherwise add the task to the schedule
    if(found == 0) TaskInputScheduleAdd(fn, input, priority, delay, period);
}

void ScheduleTask(int16_t index) {
    int16_t i;
    task_t * this_task;
    task_t * current_task;
    if(index < 0 || index >= MAX_TASK_LENGTH) RESET_OR_STOP;
    this_task = &tasks.task[index];
    if(this_task->priority == TASK_DONE) RETURN_OR_STOP;
    if(this_task->next < -1) this_task->next = -1;

    // check if task is scheduled after a rollover by checking if the task is
    // scheduled before TIME_MAX/2 and if the current time is a after TIME_MAX/2
    if(this_task->time < TIME_MAX/2 && TimeNow() > TIME_MAX/2){
        // add the time til TIME_MAX to the time then roll the timer
        this_task->time = this_task->time + (TIME_MAX - TimeNow());
        RollTimer();
        // if we do not roll the system timer then this task will be scheduled
        // for a time long before the current time and it will run right away
        // instead of running in the future. For this reason it is not
        // recommended to schedule any tasks more than TIME_MAX/2 in the future
        // (if tint_t is a uint32_t and the timer ticks every ms then this
        // equates to 24 days 20 hours 31 minutes and 23.648 seconds)
    }

    // check if there are no tasks in the schedule
    if (tasks.sch_length == 0) {
        // set first and last to this task and set next and previous to -1
        tasks.sch_first = index;
        tasks.sch_last = index;
        this_task->next = -1;
        this_task->previous = -1;
    } else {
        // scan the task queue to find where to insert this task
        // start at the last task in the list and work forward
        i = tasks.sch_last;
        // infinite loop (we will break out of it though)
        while (1) {
            if(i < 0 || i >= MAX_TASK_LENGTH) RESET_OR_STOP;
            // if this task is scheduled before the current task (i)
            current_task = &tasks.task[i];
            if (this_task->time < current_task->time) {
                // if there is no previous task then this task becomes the first
                // otherwise set the current task to the current task's previous
                // and continue
                if (current_task->previous < 0) {
                    // set first task to this one
                    tasks.sch_first = index;
                    // set the current task's previous to this one
                    current_task->previous = index;
                    // clear this task's previous (set to -1)
                    this_task->previous = -1;
                    // set this task's next to the current one
                    this_task->next = i;
                    break;
                }
                // otherwise go to the previous task (e.g. traverse the list)
                else {
                    // set the current task (i) to the current task's previous
                    i = current_task->previous;
                    // continue the scan
                    continue;
                }
            // if the priority of this task is not greater than the current task
            // then insert this task after the current task
            } else {
                // set this task's next to the current task's next
                this_task->next = current_task->next;
                // set this task's previous to the current task
                this_task->previous = i;
                // set the current task's next to this task
                current_task->next = index;
                // if the current task was the last task then update last to this task
                if (this_task->next < 0) tasks.sch_last = index;
                else if(this_task->next < MAX_TASK_LENGTH) {
                // set the next tasks previous to this task
                tasks.task[this_task->next].previous = index;
                }
                break;
            }
        }
    }
    // increment the number of tasks in the schedule
    tasks.sch_length++;
}

// remove a task from both the task queue and the schedule
// also check for any tasks that are pending
void RemoveTask(void(*fn)(void)) {
    int16_t i;
    task_t * task_ptr;
    RemoveTaskFromQueue(fn);
    RemoveTaskFromSchedule(fn);
    for(i = 0; i < MAX_TASK_LENGTH; i++) {
        task_ptr = &tasks.task[i];
        if(task_ptr->void_fn == fn) {
            task_ptr->void_fn = DEFAULT_TASK;
            if(task_ptr->priority >= 0) {
                if(task_ptr->next == TASK_LINK_QUEUE) {
                    task_ptr->count = TASK_UNLINK_QUEUE;
                    //task_ptr->next = TASK_DONE;
                }
                if(task_ptr->next == TASK_LINK_SCHEDULE) {
                    task_ptr->count = TASK_UNLINK_SCHEDULE;
                    //task_ptr->next = TASK_DONE;
                }
            }
        }
    }
}


// remove a task from both the task queue and the schedule
// also check for any tasks that are pending
void RemoveTaskInput(void(*fn)(void *), void *input) {
    int16_t i;
    task_t * task_ptr;
    RemoveTaskInputFromQueue(fn, input);
    RemoveTaskInputFromSchedule(fn, input);
    for(i = 0; i < MAX_TASK_LENGTH; i++) {
        task_ptr = &tasks.task[i];
        if(task_ptr->fn == fn && task_ptr->input == input) {
            task_ptr->fn = DEFAULT_INPUT_TASK;
            task_ptr->input = 0;
            if(task_ptr->priority >= 0) {
                if(task_ptr->next == TASK_LINK_QUEUE) {
                    task_ptr->count = TASK_UNLINK_QUEUE;
                    //task_ptr->next = TASK_DONE;
                }
                if(task_ptr->next == TASK_LINK_SCHEDULE) {
                    task_ptr->count = TASK_UNLINK_SCHEDULE;
                    //task_ptr->next = TASK_DONE;
                }
            }
        }
    }
}

void RemoveTaskFromQueue(void(*fn)(void)) {
    int16_t this_task_i, next_task_i;
    task_t * this_task;
    // loop through the task queue
    this_task_i = tasks.first;
    while(this_task_i >= 0 && this_task_i < MAX_TASK_LENGTH) {
        this_task = &tasks.task[this_task_i];
        // set the index of the next task
        next_task_i = this_task->next;
        // if the task's function pointer = the task to remove
        if(this_task->void_fn == fn) {
            // set the task to unlink
            this_task->count = TASK_UNLINK_QUEUE;
            if(tasks.to_link == TASK_DONE) tasks.to_link = this_task_i;
            else tasks.to_link = TASK_LINK_QUEUE;
        }
        // set this_task to the index of the next_task
        this_task_i = next_task_i;
    }
}


void RemoveTaskInputFromQueue(void(*fn)(void *), void *input) {
    int16_t this_task_i, next_task_i;
    task_t * this_task;
    // loop through the task queue
    this_task_i = tasks.first;
    while(this_task_i >= 0 && this_task_i < MAX_TASK_LENGTH) {
        this_task = &tasks.task[this_task_i];
        // set the index of the next task
        next_task_i = this_task->next;
        // if the task's function pointer = the task to remove
        if(this_task->fn == fn && this_task->input == input) {
            // set the task to unlink
            this_task->count = TASK_UNLINK_QUEUE;
            if(tasks.to_link == TASK_DONE) tasks.to_link = this_task_i;
            else tasks.to_link = TASK_LINK_QUEUE;
        }
        // set this_task to the index of the next_task
        this_task_i = next_task_i;
    }
}

void UnQueueTask(int16_t this_task_i) {
    int16_t previous_task_i, next_task_i;
    task_t * this_task;
    if(this_task_i < 0 || this_task_i > MAX_TASK_LENGTH) RETURN_OR_STOP;
    this_task = &tasks.task[this_task_i];
    if(this_task->priority == TASK_DONE) return;

    // set the index of the previous task
    previous_task_i = this_task->previous;
    // set the index of the next task
    next_task_i = this_task->next;

//    // if this is the only task in the queue
//    if(tasks.length <= 1) {
//        tasks.length = 0;
//        tasks.first = TASK_DONE;
//        tasks.last = TASK_DONE;
//        tasks.task[this_task].priority = TASK_DONE;
//        tasks.task[this_task].count = TASK_DONE;
//        return;
//    }
    
//    if((tasks.length <= 1 && next_task >= 0) || tasks.length == 0) {
//        while(1);
//    }

    // set the task to done
    this_task->priority = TASK_DONE;
    this_task->count = TASK_DONE;
    this_task->void_fn = DEFAULT_TASK;
    this_task->fn = DEFAULT_INPUT_TASK;
    this_task->input = 0;
    tasks.length--;
    // if there is a previous task
    if(previous_task_i >= 0 && previous_task_i < MAX_TASK_LENGTH) {
        // set the previous task's next to this task's next
        tasks.task[previous_task_i].next = next_task_i;
        // if this task's next = -1 then set last to point to the
        // previous task
        if(next_task_i < 0) tasks.last = previous_task_i;
    }// if there is no previous task then set first to the next task
    else tasks.first = next_task_i;
    // if there is a next task
    if(next_task_i >= 0 && next_task_i < MAX_TASK_LENGTH) {
        // set the next task's previous to this task's previous
        tasks.task[next_task_i].previous = previous_task_i;
        // if this task's previous = -1 then set first to point to the
        // next task
        if(previous_task_i < 0) tasks.first = next_task_i;
    }// if there is no next task then set last to the previous task
    else tasks.last = previous_task_i;
//    if(tasks.length <= 0 && tasks.first >= 0) while(1);
}

void RemoveTaskFromSchedule(void(*fn)(void)) {
    int16_t this_task_i, next_task_i;
    task_t * this_task;
    // loop through the schedule
    this_task_i = tasks.sch_first;
    while(this_task_i >= 0 && this_task_i < MAX_TASK_LENGTH) {
        this_task = &tasks.task[this_task_i];
        // set the index of the next task
        next_task_i = this_task->next;
        // if the task's function pointer = the task to remove
        if(this_task->void_fn == fn) {
            // set the task to unlink
            this_task->count = TASK_UNLINK_SCHEDULE;
            if(tasks.sch_to_link == TASK_DONE) tasks.sch_to_link = this_task_i;
            else tasks.sch_to_link = TASK_LINK_SCHEDULE;
        }
        // set this_task to the index of the next task
        this_task_i = next_task_i;
    }
}

void RemoveTaskInputFromSchedule(void(*fn)(void *), void *input) {
    int16_t this_task_i, next_task_i;
    task_t * this_task;
    // loop through the schedule
    this_task_i = tasks.sch_first;
    while(this_task_i >= 0 && this_task_i < MAX_TASK_LENGTH) {
        this_task = &tasks.task[this_task_i];
        // set the index of the next task
        next_task_i = this_task->next;
        // if the task's function pointer = the task to remove
        if(this_task->fn == fn && this_task->input == input) {
            // set the task to unlink
            this_task->count = TASK_UNLINK_SCHEDULE;
            if(tasks.sch_to_link == TASK_DONE) tasks.sch_to_link = this_task_i;
            else tasks.sch_to_link = TASK_LINK_SCHEDULE;
        }
        // set this_task to the index of the next task
        this_task_i = next_task_i;
    }
}

void UnScheduleTask(int16_t this_task_i) {
    int16_t previous_task_i, next_task_i;
    task_t * this_task;
    if(this_task_i < 0 || this_task_i >= MAX_TASK_LENGTH) RETURN_OR_STOP;
    this_task = &tasks.task[this_task_i];
    if(this_task->priority == TASK_DONE) return;
    // set the index of the previous task
    previous_task_i = this_task->previous;
    // set the index of the next task
    next_task_i = this_task->next;

    // set the task to done
    this_task->priority = TASK_DONE;
    this_task->count = TASK_DONE;
    this_task->void_fn = DEFAULT_TASK;
    this_task->fn = DEFAULT_INPUT_TASK;
    this_task->input = 0;
    tasks.sch_length--;
    // if there is a previous task
    if(previous_task_i >= 0 && previous_task_i < MAX_TASK_LENGTH) {
        // set the previous task's next to this task's next
        tasks.task[previous_task_i].next = next_task_i;
        // if this task's next = -1 then set sch_last to point to the
        // previous task
        if(next_task_i < 0) tasks.sch_last = previous_task_i;
    }// if there is no previous task then set sch_first to the next task
    else tasks.sch_first = next_task_i;
    // if there is a next task
    if(next_task_i >= 0 && next_task_i < MAX_TASK_LENGTH) {
        // set the next task's previous to this task's previous
        tasks.task[next_task_i].previous = previous_task_i;
        // if this task's previous = -1 then set sch_first to point to the
        // next task
        if(previous_task_i < 0) tasks.sch_first = next_task_i;
    }// if there is no next task then set sch_last to the previous task
    else tasks.sch_last = previous_task_i;
}

void ChangePriority(void(*fn)(void), int16_t priority) {
    int16_t i;
    task_t * task_ptr;
    // loop through all tasks and set any with the function pointer fn to have
    // a priority of priority
    for (i = 0; i < MAX_TASK_LENGTH; i++) {
        task_ptr = &tasks.task[i];
        if (task_ptr->void_fn == fn) task_ptr->priority = priority;
    }
}

void ChangePriorityInput(void(*fn)(void *), void *input, int16_t priority) {
    int16_t i;
    task_t * task_ptr;
    // loop through all tasks and set any with the function pointer fn to have
    // a priority of priority
    for (i = 0; i < MAX_TASK_LENGTH; i++) {
        task_ptr = &tasks.task[i];
        if (task_ptr->fn == fn && task_ptr->input == input) task_ptr->priority = priority;
    }
}

// must be called in order for the task management system to work
// currently implemented to execute one task on each call and load any
// scheduled tasks as needed

void SystemTick(void) {
    int16_t i;
    task_t * task_ptr;
#ifdef PERMANENT_TASKS
    static tint_t check_perm_time = 0;
#endif
    static tint_t last_time = 0;
    static tint_t service_tasks_time = 0;

    // check if any tasks need to be linked into the queue
    if (tasks.to_link != TASK_DONE) {
        // temp save and clear to_link so it can be set properly if QueueTask
        // is interrupted
        i = tasks.to_link;
        tasks.to_link = TASK_DONE;
        if (i >= 0 && i < MAX_TASK_LENGTH) {
            task_ptr = &tasks.task[i];
            // double check that the task needs to be linked and link the task
            if (task_ptr->next == TASK_LINK_QUEUE) QueueTask(i);
            if (task_ptr->count == TASK_UNLINK_QUEUE) UnQueueTask(i);
        } else {
            // go through the whole task array and look for any tasks needing to
            // be linked into the task queue
            for (i = 0; i < MAX_TASK_LENGTH; i++) {
                if (tasks.task[i].next == TASK_LINK_QUEUE) QueueTask(i);
                if (tasks.task[i].count == TASK_UNLINK_QUEUE) UnQueueTask(i);
            }
        }
    }

    // set i to the first task
    i = tasks.first;
    // if there is a task in queue then run it
    if (i >= 0 && i < MAX_TASK_LENGTH) {
        task_ptr = &tasks.task[i];
        // run the task
        task_ptr->fn(task_ptr->input);
		task_ptr->void_fn();

        // set the next task to run
        tasks.first = task_ptr->next;
        if (tasks.first >= 0 && tasks.first < MAX_TASK_LENGTH) {
            tasks.task[tasks.first].previous = TASK_DONE;
        }
        // check if the task needs to be added to the schedule (period > 0)
        if (task_ptr->period > 0 && task_ptr->count != 0) {
            if (task_ptr->count > 0) {
                task_ptr->count--;
            }
            // if remove task was called after the above if but before count--
            // then count could be corrupted, if so don't schedule the task
            if (task_ptr->count < TASK_DONE) {
                // clear this task
                task_ptr->priority = TASK_DONE;
                task_ptr->void_fn = DEFAULT_TASK;
                task_ptr->fn = DEFAULT_INPUT_TASK;
                task_ptr->input = 0;
                // set available to the task that just ran
                tasks.available = i;
                task_ptr->count = TASK_DONE;
            } else {
                // increment the task's time by period
                task_ptr->time += task_ptr->period;
                // schedule the task
                ScheduleTask(i);
            }
        }// otherwise set this task to inactive (priority = TASK_DONE) and set
            // tasks.available to this task
        else {
            // clear this task
            task_ptr->priority = TASK_DONE;
            task_ptr->void_fn = DEFAULT_TASK;
            task_ptr->fn = DEFAULT_INPUT_TASK;
            task_ptr->input = 0;
            // set available to the task that just ran
            tasks.available = i;
        }
        // if length is greater than max length then update max length
        if ((tasks.length + tasks.sch_length) > tasks.max_length)
            tasks.max_length = tasks.length + tasks.sch_length;
        // decrement the length of the queue since it is one shorter now
        tasks.length--;
        if (tasks.length < 1) tasks.last = TASK_DONE;
    }

    // check if any tasks need to be linked into the schedule
    if (tasks.sch_to_link != TASK_DONE) {
        // temp save and clear to_link so it can be set properly if ScheduleTask
        // is interrupted
        i = tasks.sch_to_link;
        tasks.sch_to_link = TASK_DONE;
        if (i >= 0 && i < MAX_TASK_LENGTH) {
            task_ptr = &tasks.task[i];
            // double check that the task needs to be linked and link the task
            if (task_ptr->next == TASK_LINK_SCHEDULE) ScheduleTask(i);
            if (task_ptr->count == TASK_UNLINK_SCHEDULE) UnScheduleTask(i);
        } else {
            // go through the whole task array and look for any tasks needing to
            // be linked into the task queue
            for (i = 0; i < MAX_TASK_LENGTH; i++) {
                if (tasks.task[i].next == TASK_LINK_SCHEDULE) ScheduleTask(i);
                if (tasks.task[i].count == TASK_UNLINK_SCHEDULE) UnScheduleTask(i);
            }
        }
    }

    if (TimeNow() != last_time) {
        last_time = TimeNow();
        // if the next task time is scheduled <= the current time
        // add the task to the queue and update the next task scheduled
        // repeat until the next schedule task is in the future
        while (tasks.sch_first >= 0 && tasks.sch_first < MAX_TASK_LENGTH) {
            // if the task is scheduled in the future then break
            if (tasks.task[tasks.sch_first].time > TimeNow()) break;
            // set i to the first scheduled task (soonest to run)
            i = tasks.sch_first;
            // update the first scheduled task to the current first's next task
            tasks.sch_first = tasks.task[i].next;
            if (tasks.sch_first >= 0 && tasks.sch_first < MAX_TASK_LENGTH) {
                tasks.task[tasks.sch_first].previous = TASK_DONE;
            }
            // decrement the schedule length
            tasks.sch_length--;
            if (tasks.sch_length < 1) tasks.sch_last = TASK_DONE;
            // queue the task
            QueueTask(i);
        }

        if (TimeSince(service_tasks_time) > SERVICE_TASKS_PERIOD) {
            service_tasks_time = TimeNow();
            ServiceTasks();
        }
#ifdef PERMANENT_TASKS
        if (TimeSince(check_perm_time) > CHECK_PERM_TASKS_PERIOD) {
            check_perm_time = TimeNow();
            CheckPermSchedule();
        }
#endif
    }
}

void RollTimer(void) {
    int i;
    tint_t time;
    task_t * task_ptr;
    // get the current time
    time = TimeNow();
    // for each task
    for (i = 0; i < MAX_TASK_LENGTH; i++) {
        task_ptr = &tasks.task[i];
        // if the task is enabled
        if (task_ptr->priority != TASK_DONE) {
            // if time is in the future then roll it
            // otherwise just let it be as TimeSince will handle the roll
            if (task_ptr->time > time) {
                // rolled task time = task time - current time
                // since current time is about to be set to 0
                task_ptr->time = task_ptr->time - time;
            }
        }
    }

    // reset the system time and set the rollover time
    TimerRoll();
}

// delay a set number of milliseconds but call SystemTick() while we wait so
// we will run system processes while we wait

void WaitMs(tint_t wait) {
    tint_t time;
    // get the current time
    time = TimeNow();
    // while time since time is less than or equal to wait
    while (TimeSince(time) <= wait) {
        // call SystemTick()
        SystemTick();
    }
}

void GetAvailableTask(int16_t priority) {
    int16_t i;
    task_t * last_task;
    // loop through the task queue array and find the first available slot
    for (i = 0; i < MAX_TASK_LENGTH; i++) {
        // if the priority is TASK_DONE then it is available
        if (tasks.task[i].priority == TASK_DONE) {
            // set tasks.available and break
            tasks.available = i;
            return;
        }
    }

    // if no task slot is found then give an error
    // if the last slot has a task with a lower priority than this one
    // then drop the last task and use its slot for this task.
    if (tasks.last >= 0 && tasks.last < MAX_TASK_LENGTH) {
        last_task = &tasks.task[tasks.last];
        if (last_task->priority < priority && last_task->period == 0) {
            if (tasks.first != tasks.last) {
                // set available to last
                tasks.available = tasks.last;
                // set the last to be the task before the last task
                tasks.last = last_task->previous;
                // must update next field of new last task (if exists)
                if (tasks.last >= 0 && tasks.last < MAX_TASK_LENGTH) {
                    last_task->next = TASK_DONE;
                }
            }
            // log an error
#ifdef TASK_LOGGING_ENABLE
            LogMsg(TASK, ERROR, "Task queue full! Last task purged");
#endif
            RETURN_OR_STOP;
        }            // otherwise this task must be dropped
          else {
            // log an error
#ifdef TASK_LOGGING_ENABLE
            LogMsg(TASK, ERROR, "Task queue full! Task dropped");
#endif
            RETURN_OR_STOP;
        }
    }
    //    // check if there are any lower priority single shot tasks in the schedule
    //    for (i = 0; i < MAX_TASK_LENGTH; i++) {
    //        // if the priority is TASK_DONE then it is available
    //        if (tasks.task[i].priority < priority) {
    //            if(tasks.task[i].period == 0) {
    //                UnScheduleTask(i);
    //                // set tasks.available and break
    //                tasks.available = i;
    //                return;
    //            }
    //        }
    //    }
#ifdef TASK_LOGGING_ENABLE
    LogMsg(TASK, ERROR, "Task schedule full! Task dropped");
#else
    //Nop();
#endif
}

uint8_t IsTaskScheduled(void(*fn)(void)) {
	// changed from uint to int on 3/26/2014
    int16_t i, j;
    task_t * task_ptr;
    j = 0;
    i = tasks.sch_first;
    while (i >= 0 && i < MAX_TASK_LENGTH) {
        task_ptr = &tasks.task[i];
        if (task_ptr->void_fn == fn) return 1;
        i = task_ptr->next;
        j++;
        if (j >= MAX_TASK_LENGTH) RESET_OR_STOP;
    }
    j = 0;
    i = tasks.first;
    while (i >= 0 && i < MAX_TASK_LENGTH) {
        task_ptr = &tasks.task[i];
        if (task_ptr->void_fn == fn) return 1;
        i = task_ptr->next;
        j++;
        if (j >= MAX_TASK_LENGTH) RESET_OR_STOP;
    }
    return 0;
}


uint8_t IsTaskInputScheduled(void(*fn)(void *), void *input) {
	// changed from uint to int on 3/26/2014
    int16_t i, j;
    task_t * task_ptr;
    j = 0;
    i = tasks.sch_first;
    while (i >= 0 && i < MAX_TASK_LENGTH) {
        task_ptr = &tasks.task[i];
        if (task_ptr->fn == fn && task_ptr->input == input) return 1;
        i = task_ptr->next;
        j++;
        if (j >= MAX_TASK_LENGTH) RESET_OR_STOP;
    }
    j = 0;
    i = tasks.first;
    while (i >= 0 && i < MAX_TASK_LENGTH) {
        task_ptr = &tasks.task[i];
        if (task_ptr->fn == fn && task_ptr->input == input) return 1;
        i = task_ptr->next;
        j++;
        if (j >= MAX_TASK_LENGTH) RESET_OR_STOP;
    }
    return 0;
}

// a service routine to make sure no tasks need to be linked into the queue
// or schedule and to increase the priority of any tasks that have been in the
// queue too long

void ServiceTasks(void) {
    int16_t i;
    int16_t j;
    task_t * task_ptr;
    for (i = 0; i < MAX_TASK_LENGTH; i++) {
        task_ptr = &tasks.task[i];
        if (task_ptr->priority >= 0) {
            if (task_ptr->next == TASK_LINK_QUEUE) QueueTask(i);
            if (task_ptr->next == TASK_LINK_SCHEDULE) ScheduleTask(i);
            if (task_ptr->period > 0
                    && tasks.first != i
                    && tasks.sch_first != i
                    && task_ptr->next == TASK_DONE
                    && task_ptr->previous == TASK_DONE) {
                task_ptr->time = TimeNow() + task_ptr->period;
                ScheduleTask(i);
            }
        }
    }
#ifdef TASK_BUMP_PRIORITY_TIME
    i = tasks.last;
    j = 0;
    while (i >= 0 && i < MAX_TASK_LENGTH) {
        task_ptr = &tasks.task[i];
        if (TimeSince(task_ptr->time) > TASK_BUMP_PRIORITY_TIME)
            task_ptr->priority++;
        i = task_ptr->previous;
        j++;
        if (j > MAX_TASK_LENGTH) RETURN_OR_STOP;
    }
#endif
}

#ifdef PERMANENT_TASKS

void CheckPermSchedule(void) {
#ifdef PERM_TASK_1_FN
    if (IsTaskScheduled(PERM_TASK_1_FN) == 0) {
        TaskScheduleAdd(PERM_TASK_1_FN, PERM_TASK_1_PRIORITY, PERM_TASK_1_TIME, PERM_TASK_1_PERIOD);
    }
#endif
#ifdef PERM_TASK_2_FN
    if (IsTaskScheduled(PERM_TASK_2_FN) == 0) {
        TaskScheduleAdd(PERM_TASK_2_FN, PERM_TASK_2_PRIORITY, PERM_TASK_2_TIME, PERM_TASK_2_PERIOD);
    }
#endif
#ifdef PERM_TASK_3_FN
    if (IsTaskScheduled(PERM_TASK_3_FN) == 0) {
        TaskScheduleAdd(PERM_TASK_3_FN, PERM_TASK_3_PRIORITY, PERM_TASK_3_TIME, PERM_TASK_3_PERIOD);
    }
#endif
#ifdef PERM_TASK_4_FN
    if (IsTaskScheduled(PERM_TASK_4_FN) == 0) {
        TaskScheduleAdd(PERM_TASK_4_FN, PERM_TASK_4_PRIORITY, PERM_TASK_4_TIME, PERM_TASK_4_PERIOD);
    }
#endif
#ifdef PERM_TASK_5_FN
    if (IsTaskScheduled(PERM_TASK_5_FN) == 0) {
        TaskScheduleAdd(PERM_TASK_5_FN, PERM_TASK_5_PRIORITY, PERM_TASK_5_TIME, PERM_TASK_5_PERIOD);
    }
#endif
#ifdef PERM_TASK_6_FN
    if (IsTaskScheduled(PERM_TASK_6_FN) == 0) {
        TaskScheduleAdd(PERM_TASK_6_FN, PERM_TASK_6_PRIORITY, PERM_TASK_6_TIME, PERM_TASK_6_PERIOD);
    }
#endif
#ifdef PERM_TASK_7_FN
    if (IsTaskScheduled(PERM_TASK_7_FN) == 0) {
        TaskScheduleAdd(PERM_TASK_7_FN, PERM_TASK_7_PRIORITY, PERM_TASK_7_TIME, PERM_TASK_7_PERIOD);
    }
#endif
#ifdef PERM_TASK_8_FN
    if (IsTaskScheduled(PERM_TASK_8_FN) == 0) {
        TaskScheduleAdd(PERM_TASK_8_FN, PERM_TASK_8_PRIORITY, PERM_TASK_8_TIME, PERM_TASK_8_PERIOD);
    }
#endif
#ifdef PERM_TASK_9_FN
    if (IsTaskScheduled(PERM_TASK_9_FN) == 0) {
        TaskScheduleAdd(PERM_TASK_9_FN, PERM_TASK_9_PRIORITY, PERM_TASK_9_TIME, PERM_TASK_9_PERIOD);
    }
#endif
#ifdef PERM_TASK_10_FN
    if (IsTaskScheduled(PERM_TASK_10_FN) == 0) {
        TaskScheduleAdd(PERM_TASK_10_FN, PERM_TASK_10_PRIORITY, PERM_TASK_10_TIME, PERM_TASK_10_PERIOD);
    }
#endif
}
#endif // PERMANENT_TASKS

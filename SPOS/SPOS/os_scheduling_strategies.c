#include "os_scheduling_strategies.h"
#include "defines.h"

#include "util.h"
#include <stdlib.h>

typedef struct {
	uint8_t timeSlice;
	Age age[MAX_NUMBER_OF_PROCESSES];
} SchedulingInformation;

SchedulingInformation schedulingInfo;

extern Process os_processes[MAX_NUMBER_OF_PROCESSES];

/*!
 *  Reset the scheduling information for a specific strategy
 *  This is only relevant for RoundRobin and InactiveAging
 *  and is done when the strategy is changed through os_setSchedulingStrategy
 *
 * \param strategy  The strategy to reset information for
 */
void os_resetSchedulingInformation(SchedulingStrategy strategy) {
    if(strategy == OS_SS_ROUND_ROBIN){
		schedulingInfo.timeSlice = os_processes[os_getCurrentProc()].priority;
	} else if (strategy == OS_SS_INACTIVE_AGING){
		for(uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
			schedulingInfo.age[i] = 0;
		}
	} 
}

/*!
 *  Reset the scheduling information for a specific process slot
 *  This is necessary when a new process is started to clear out any
 *  leftover data from a process that previously occupied that slot
 *
 *  \param id  The process slot to erase state for
 */
void os_resetProcessSchedulingInformation(ProcessID id) {
    schedulingInfo.age[id] = 0;
}



/*!
 *  This function implements the even strategy. Every process gets the same
 *  amount of processing time and is rescheduled after each scheduler call
 *  if there are other processes running other than the idle process.
 *  The idle process is executed if no other process is ready for execution
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the even strategy.
 */
ProcessID os_Scheduler_Even(Process const processes[], ProcessID current) {
	ProcessID next = current + 1;
	uint8_t returnIdle = 1;
	for(uint8_t i = next; i < MAX_NUMBER_OF_PROCESSES; i++){
		if(processes[i].state == OS_PS_READY){
			next = i;
			returnIdle = 0;
			break;
		}
	}
	for(uint8_t j = 1; j <= current; j++){
		if(processes[j].state == OS_PS_READY && returnIdle == 1){
			next = j;
			returnIdle = 0;
			break;
		}
	}
	if (returnIdle == 1){
		return 0;
	} else return next;
}

/*!
 *  This functions counts how many processes that are ready to run.
 *  It should help with implementation of the RANDOM scheduling strategy.
 *  The idle process and already running processes will be ignored.
 * 
 *  \param processes An array holding all processes.
 *  \return The number of runnable processes, except the idle process.
 */
uint8_t os_countReadyProcesses(Process const processes[]) {
	uint8_t count = 0;
	for (uint8_t pid = 1; pid < MAX_NUMBER_OF_PROCESSES; pid++){ //Starts from 1 as idle process is ignored
		if(processes[pid].state == OS_PS_READY){
			count++;
		}
	}
	return count;
}

/*!
 *  This function implements the random strategy. The next process is chosen based on
 *  the result of a pseudo random number generator.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the random strategy.
 */
ProcessID os_Scheduler_Random(Process const processes[], ProcessID current) {
	if(os_Scheduler_Even(processes, current) == 0){
		return 0;
	}
	ProcessID nextProcessID = current;
    uint8_t count = os_countReadyProcesses(processes); //Number of ready processes
	uint8_t randomNumber = 1 + (rand() % count); 
	for(uint16_t i = 0; i < randomNumber; i++){
		nextProcessID = os_Scheduler_Even(processes, current);
	};
	return nextProcessID;
}

/*!
 *  This function implements the round-robin strategy. In this strategy, process priorities
 *  are considered when choosing the next process. A process stays active as long its time slice
 *  does not reach zero. This time slice is initialized with the priority of each specific process
 *  and decremented each time this function is called. If the time slice reaches zero, the even
 *  strategy is used to determine the next process to run.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the round robin strategy.
 */
#define timeSlice schedulingInfo.timeSlice
ProcessID os_Scheduler_RoundRobin(Process const processes[], ProcessID current) {
	if(os_Scheduler_Even(processes, current) == 0){
		return 0;
	} else {
		timeSlice--;
		if(processes[current].state == OS_PS_UNUSED || timeSlice == 0){
			ProcessID next = os_Scheduler_Even(processes, current);
			timeSlice = os_processes[next].priority;
			return next;
		} else {
			return current;
		}
	}
}
#undef timeSlice

/*!
 *  This function realizes the inactive-aging strategy. In this strategy a process specific integer ("the age") is used to determine
 *  which process will be chosen. At first, the age of every waiting process is increased by its priority. After that the oldest
 *  process is chosen. If the oldest process is not distinct, the one with the highest priority is chosen. If this is not distinct
 *  as well, the one with the lower ProcessID is chosen. Before actually returning the ProcessID, the age of the process who
 *  is to be returned is reset to its priority.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the inactive-aging strategy.
 */
ProcessID os_Scheduler_InactiveAging(Process const processes[], ProcessID current) {
	if(os_Scheduler_Even(processes, current) == 0){
		return 0;
	} else {
	    for(uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
			// This loop "ages" all processes except the current one
		    if(processes[i].state == OS_PS_READY && i != current){
			    schedulingInfo.age[i] += processes[i].priority;
		    }
	    } 
	    ProcessID next = 1;
	    for(uint8_t i = 1; i < MAX_NUMBER_OF_PROCESSES; i++){
			/* This loop chooses the next processes based on the following criteria in descending order:
			 * 1. Oldest age
			 * 2. Highest priority
			 * 3. Smallest Process ID
			 */
		    if(processes[i].state == OS_PS_READY){
			    if (schedulingInfo.age[next] < schedulingInfo.age[i]){
				    next = i; // i is older than next, update next
			    } else if (schedulingInfo.age[next] == schedulingInfo.age[i] && processes[next].priority < processes[i].priority){
					/* If ages of i and next are equal, then update next only if its priority is smaller than that of i.
					 * If their priorities are equal, we choose next because next is always <= i and don't update it. 
					 */
				    next = i;  
			    }
		    }
	    } 
	    schedulingInfo.age[next] = processes[next].priority; // Reset the age of the chosen process
	    return next;
	}
}
/*!
 *  This function realizes the run-to-completion strategy.
 *  As long as the process that has run before is still ready, it is returned again.
 *  If  it is not ready, the even strategy is used to determine the process to be returned
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the run-to-completion strategy.
 */
ProcessID os_Scheduler_RunToCompletion(Process const processes[], ProcessID current) {
	if(os_Scheduler_Even(processes, current) == 0){
		return 0;
	}
    if (processes[current].state == OS_PS_READY){
		return current; //If it's still ready let it run
	} else return os_Scheduler_Even(processes, current); //Else we get the next one like EVEN
}

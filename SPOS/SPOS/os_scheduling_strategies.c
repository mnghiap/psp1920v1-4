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
 *  This function returns the next process in the process array that is ready to run,
 *  starting from the current process. It only returns 0 (the idle process) if no other  
 *  process except than the idle process is in ready state.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process after current process that is in ready state. 
 */
#define length MAX_NUMBER_OF_PROCESSES //convenient naming
ProcessID os_getNextReadyProcess(Process const processes[], ProcessID current) {
	ProcessID afterCurrent = (current + 1) % length;
	ProcessID nextProcessID = afterCurrent; //The search starts at the process after the current one
	do {
		if (processes[nextProcessID].state != OS_PS_READY){ //Not ready
			nextProcessID = (nextProcessID + 1) % length; //Go to next process
		} else if (nextProcessID == 0){
			nextProcessID = (nextProcessID + 1) % length; //We ignore the idle process and continue the search
		} else {
			return nextProcessID; //Such process found
		}
	} while (nextProcessID != afterCurrent);
	return 0; //All process checked, no ready found other than idle
}
#undef length

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
    return os_getNextReadyProcess(processes, current); //Straightforward go to the next ready one
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
	ProcessID nextProcessID = current;
    uint8_t count = os_countReadyProcesses(processes); //Number of ready processes
	uint8_t randomNumber = rand() % count; 
	for(uint16_t i = 0; i < randomNumber; i++){
		nextProcessID = os_getNextReadyProcess(processes, nextProcessID);
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
ProcessID os_Scheduler_RoundRobin(Process const processes[], ProcessID current) {
    schedulingInfo.timeSlice--; // Time slice decreased
	if(schedulingInfo.timeSlice > 0){ // There's still time for this process
		return current; 
	} else { // There's no more time for current
		ProcessID nextProcess = os_getNextReadyProcess(processes, current); // Choose the next one like EVEN
		schedulingInfo.timeSlice = processes[nextProcess].priority; // Give the new process a time slice
		return nextProcess;
	}
}

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
	if(os_getNextReadyProcess(processes, current) == 0){ // If no process other than idle is ready
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
    if (processes[current].state == OS_PS_READY){
		return current; //If it's still ready let it run
	} else return os_getNextReadyProcess(processes, current); //Else we get the next one like EVEN
}

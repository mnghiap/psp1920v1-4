#include "os_scheduling_strategies.h"
#include "defines.h"

#include <stdlib.h>

/*!
 *  Reset the scheduling information for a specific strategy
 *  This is only relevant for RoundRobin and InactiveAging
 *  and is done when the strategy is changed through os_setSchedulingStrategy
 *
 * \param strategy  The strategy to reset information for
 */
void os_resetSchedulingInformation(SchedulingStrategy strategy) {
    // This is a presence task
}

/*!
 *  Reset the scheduling information for a specific process slot
 *  This is necessary when a new process is started to clear out any
 *  leftover data from a process that previously occupied that slot
 *
 *  \param id  The process slot to erase state for
 */
void os_resetProcessSchedulingInformation(ProcessID id) {
    // This is a presence task
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
ProcessID os_getNextReadyProcess(Process const processes[], ProcessID current) {
	uint8_t length = MAX_NUMBER_OF_PROCESSES; //Convenient naming
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
	for(uint8_t i = 0; i < randomNumber; i++){
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
    // This is a presence task
    return 0;
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
    // This is a presence task
    return 0;
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
    // This is a presence task
    return 0;
}

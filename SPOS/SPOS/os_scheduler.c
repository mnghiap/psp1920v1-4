#include "os_scheduler.h"
#include "util.h"
#include "os_input.h"
#include "os_scheduling_strategies.h"
#include "os_taskman.h"
#include "os_core.h"
#include "string.h"
#include "lcd.h"
#include "os_memheap_drivers.h"
#include "os_memory.h"

#include <avr/interrupt.h>
#include <stdbool.h>

//----------------------------------------------------------------------------
// Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//! Array of states for every possible process
Process os_processes[MAX_NUMBER_OF_PROCESSES] = {{OS_PS_UNUSED, {0}, 0, 0}};

//! Array of function pointers for every registered program
Program* os_programs[MAX_NUMBER_OF_PROGRAMS];

//! Index of process that is currently executed (default: idle)
ProcessID currentProc;

//----------------------------------------------------------------------------
// Private variables
//----------------------------------------------------------------------------

//! Currently active scheduling strategy
SchedulingStrategy currentStrat;

//! Count of currently nested critical sections
uint8_t criticalSectionCount = 0;

//! Used to auto-execute programs.
uint16_t os_autostart;

//----------------------------------------------------------------------------
// Private function declarations
//----------------------------------------------------------------------------

//! ISR for timer compare match (scheduler)
ISR(TIMER2_COMPA_vect) __attribute__((naked));

//----------------------------------------------------------------------------
// Function definitions
//----------------------------------------------------------------------------

/*!
 *  Timer interrupt that implements our scheduler. Execution of the running
 *  process is suspended and the context saved to the stack. Then the periphery
 *  is scanned for any input events. If everything is in order, the next process
 *  for execution is derived with an exchangeable strategy. Finally the
 *  scheduler restores the next process for execution and releases control over
 *  the processor to that process.
 */
ISR(TIMER2_COMPA_vect) {
    // 1. store PC
    //  - done implicitly by jumping into ISR
   
    // 2 + 3. Store context and SP of current process 
    saveContext();
    os_processes[os_getCurrentProc()].sp.as_int = SP;
    
    // 4. set SP to scheduler stack
    SP = BOTTOM_OF_ISR_STACK;
	
	// Calculate and save Stack Checksum
	os_processes[os_getCurrentProc()].checksum = os_getStackChecksum(os_getCurrentProc());
	
    // 5. set cur. process to OS_PS_READY if it has not terminated yet
	if(os_processes[os_getCurrentProc()].state == OS_PS_RUNNING){
        os_processes[os_getCurrentProc()].state = OS_PS_READY;
	}
	
	// Open task manager if ESC + ENTER are pressed
	if(os_getInput() == 0b1001){
		os_waitForNoInput();
		os_taskManMain();
	}
    
    // 6. get next process depending on scheduling strategy
	do {
		executeScheduler(os_getSchedulingStrategy());
	} while (os_processes[os_getCurrentProc()].state == OS_PS_UNUSED);
	
	// Verify checksum to avoid stack inconsistence
	StackChecksum currentChecksum = os_getStackChecksum(os_getCurrentProc());
	if(currentChecksum != os_processes[os_getCurrentProc()].checksum){
		os_error("Stack inconsistency");
	}
	
    // 7 + 8 + 9. restore new process (SP and context) and set state
	if (os_processes[os_getCurrentProc()].state == OS_PS_UNUSED)
		os_error("ISR: UNUSED Process selected");
    os_processes[os_getCurrentProc()].state = OS_PS_RUNNING;
    SP = os_processes[os_getCurrentProc()].sp.as_int;
	restoreContext();
	
    // 10. automatic return
}

/*!
 *  Used to register a function as program. On success the program is written to
 *  the first free slot within the os_programs array (if the program is not yet
 *  registered) and the index is returned. On failure, INVALID_PROGRAM is returned.
 *  Note, that this function is not used to register the idle program.
 *
 *  \param program The function you want to register.
 *  \return The index of the newly registered program.
 */
ProgramID os_registerProgram(Program* program) {
    ProgramID slot = 0;

    // Find first free place to put our program
    while (os_programs[slot] &&
           os_programs[slot] != program && slot < MAX_NUMBER_OF_PROGRAMS) {
        slot++;
    }

    if (slot >= MAX_NUMBER_OF_PROGRAMS) {
        return INVALID_PROGRAM;
    }

    os_programs[slot] = program;
    return slot;
}

/*!
 *  Used to check whether a certain program ID is to be automatically executed at
 *  system start.
 *
 *  \param programID The program to be checked.
 *  \return True if the program with the specified ID is to be auto started.
 */
bool os_checkAutostartProgram(ProgramID programID) {
    return !!(os_autostart & (1 << programID));
}

/*!
 *  This is the idle program. The idle process owns all the memory
 *  and processor time no other process wants to have.
 */
PROGRAM(0, AUTOSTART) {
    for (;;) {
        lcd_clear();
        lcd_writeProgString(PSTR("."));
        delayMs(DEFAULT_OUTPUT_DELAY);
    }
}

/*!
 * Lookup the main function of a program with id "programID".
 *
 * \param programID The id of the program to be looked up.
 * \return The pointer to the according function, or NULL if programID is invalid.
 */
Program* os_lookupProgramFunction(ProgramID programID) {
    // Return NULL if the index is out of range
    if (programID >= MAX_NUMBER_OF_PROGRAMS) {
        return NULL;
    }

    return os_programs[programID];
}

/*!
 * Lookup the id of a program.
 *
 * \param program The function of the program you want to look up.
 * \return The id to the according slot, or INVALID_PROGRAM if program is invalid.
 */
ProgramID os_lookupProgramID(Program* program) {
    ProgramID i;

    // Search program array for a match
    for (i = 0; i < MAX_NUMBER_OF_PROGRAMS; i++) {
        if (os_programs[i] == program) {
            return i;
        }
    }

    // If no match was found return INVALID_PROGRAM
    return INVALID_PROGRAM;
}

bool os_kill(ProcessID pid){
	os_enterCriticalSection();
	if(pid == 0 || pid >= MAX_NUMBER_OF_PROCESSES 
            || os_getProcessSlot(pid)->state == OS_PS_UNUSED){
        //os_errorPStr(PSTR("Kill: Process is invalid"));
		os_leaveCriticalSection();
		return false;
	} 
    os_getProcessSlot(pid)->state = OS_PS_UNUSED;
    for (uint8_t i = 0; i < os_getHeapListLength(); i++)
	    os_freeProcessMemory(os_lookupHeap(i), pid);
        
    if (pid == os_getCurrentProc()) {
        // reset critical section count back to 1, to 
        // prevent blocking 
        criticalSectionCount = 1;
        //currentProc = 0;
        os_leaveCriticalSection();
        while (1);
    } 
    
	os_leaveCriticalSection();
	return true;
}

void os_dispatcher(void){
	// determine the active process and its program function to give it to a function pointer
	void (*program_pointer)(void) = os_lookupProgramFunction(os_processes[os_getCurrentProc()].progID);
	
	// call the program function with the pointer
	(*program_pointer)();
	
	// kill the process after it has been terminated
	os_kill(os_getCurrentProc());
}

/*!
 *  This function is used to execute a program that has been introduced with
 *  os_registerProgram.
 *  A stack will be provided if the process limit has not yet been reached.
 *  In case of an error, an according message will be displayed on the LCD.
 *  This function is multitasking safe. That means that programs can repost
 *  themselves, simulating TinyOS 2 scheduling (just kick off interrupts ;) ).
 *
 *  \param programID The program id of the program to start (index of os_programs).
 *  \param priority A priority ranging 0..255 for the new process:
 *                   - 0 means least favorable
 *                   - 255 means most favorable
 *                  Note that the priority may be ignored by certain scheduling
 *                  strategies.
 *  \return The index of the new process (throws error on failure and returns
 *          INVALID_PROCESS as specified in defines.h).
 */
ProcessID os_exec(ProgramID programID, Priority priority) {
	
	os_enterCriticalSection();
    // 1. find free space in `os_processes`
    ProcessID pid = 0;
    for (pid = 0; pid <= MAX_NUMBER_OF_PROCESSES; pid++) {
        if (pid == MAX_NUMBER_OF_PROCESSES){  // element that isn't available in `os_programs`
            os_leaveCriticalSection();
			return INVALID_PROCESS;
		}
        if (os_processes[pid].state == OS_PS_UNUSED)
            break;  // first null elem. found
    }

    // 2. load program index
    Program* program_ptr = &(os_dispatcher);
    if (program_ptr == NULL){
		os_leaveCriticalSection();
        return INVALID_PROCESS;
	}
	    
    // 3. store program index, state and priority
	os_processes[pid] = (Process){
		.sp = {PROCESS_STACK_BOTTOM(pid) - (32 + 1 + 2)},
        .state = OS_PS_READY,
        .progID = programID,
        .priority = priority,
    };
    
	// 4. prepare process stack
    StackPointer ptr = {.as_ptr = (uint8_t*)program_ptr };
    
    os_processes[pid].sp.as_ptr[35 - 0] = (uint8_t)(ptr.as_int & 0xFF);
    os_processes[pid].sp.as_ptr[35 - 1] = (uint8_t)(ptr.as_int >> 8);
    for (int i = 0; i < 33; i++)
        os_processes[pid].sp.as_ptr[35 - i-2] = 0;
		
	// 5. initialize stack checksum
	os_processes[pid].checksum = os_getStackChecksum(pid);
	
	os_resetProcessSchedulingInformation(pid);
	
    os_leaveCriticalSection();
	
    return pid;
	
}

/*!
 *  If all processes have been registered for execution, the OS calls this
 *  function to start the idle program and the concurrent execution of the
 *  applications.
 */
void os_startScheduler(void) {
    currentProc = 0;
    os_processes[0].state = OS_PS_RUNNING;
    SP = os_processes[0].sp.as_int;
    restoreContext();
}

/*!
 *  In order for the Scheduler to work properly, it must have the chance to
 *  initialize its internal data-structures and register.
 */
void os_initScheduler(void) {
    // 1. prepare os_processes
    memset(os_processes, OS_PS_UNUSED, sizeof(os_processes));
    
    // 2. set autostart programs
    for (int i = 0; i < MAX_NUMBER_OF_PROGRAMS; i++)
        if (os_checkAutostartProgram(i)) 
            os_exec(i, DEFAULT_PRIORITY);
}

/*!
 *  A simple getter for the slot of a specific process.
 *
 *  \param pid The processID of the process to be handled
 *  \return A pointer to the memory of the process at position pid in the os_processes array.
 */
Process* os_getProcessSlot(ProcessID pid) {
    return os_processes + pid;
}

/*!
 *  A simple getter for the slot of a specific program.
 *
 *  \param programID The ProgramID of the process to be handled
 *  \return A pointer to the function pointer of the program at position programID in the os_programs array.
 */
Program** os_getProgramSlot(ProgramID programID) {
    return os_programs + programID;
}

/*!
 *  A simple getter to retrieve the currently active process.
 *
 *  \return The process id of the currently active process.
 */
ProcessID os_getCurrentProc(void) {
    return currentProc;
}

/*!
 *  This function return the the number of currently active process-slots.
 *
 *  \returns The number currently active (not unused) process-slots.
 */
uint8_t os_getNumberOfActiveProcs(void) {
    uint8_t num = 0;

    ProcessID i = 0;
    do {
        num += os_getProcessSlot(i)->state != OS_PS_UNUSED;
    } while (++i < MAX_NUMBER_OF_PROCESSES);

    return num;
}

/*!
 *  This function returns the number of currently registered programs.
 *
 *  \returns The amount of currently registered programs.
 */
uint8_t os_getNumberOfRegisteredPrograms(void) {
    uint8_t i;
    for (i = 0; i < MAX_NUMBER_OF_PROGRAMS && *(os_getProgramSlot(i)); i++);
    // Note that this only works because programs cannot be unregistered.
    return i;
}

/*!
 *  Sets the current scheduling strategy.
 *
 *  \param strategy The strategy that will be used after the function finishes.
 */
void os_setSchedulingStrategy(SchedulingStrategy strategy) {
    currentStrat = strategy;
	os_resetSchedulingInformation(currentStrat);
}

/*!
 *  This is a getter for retrieving the current scheduling strategy.
 *
 *  \return The current scheduling strategy.
 */
SchedulingStrategy os_getSchedulingStrategy(void) {
    return currentStrat;
}

/*!
 *  Set currentProc to the next process in line, depending 
 *  on the current scheduling strategy
 */
void executeScheduler(SchedulingStrategy strategy) {
    ProcessID (*fun_ptr)(Process const[], ProcessID) = os_Scheduler_Even;
  
    switch(strategy) {
        case OS_SS_EVEN: fun_ptr = os_Scheduler_Even; break;
        case OS_SS_RANDOM: fun_ptr = os_Scheduler_Random; break;
        case OS_SS_RUN_TO_COMPLETION: fun_ptr = os_Scheduler_RunToCompletion; break;
        case OS_SS_ROUND_ROBIN: fun_ptr = os_Scheduler_RoundRobin; break;
        case OS_SS_INACTIVE_AGING: fun_ptr = os_Scheduler_InactiveAging; break;
        default: os_errorPStr(PSTR("Unknown scheduling strategy"));
    }   
    
    currentProc = fun_ptr(os_processes, currentProc);
}


/*!
 *  Enters a critical code section by disabling the scheduler if needed.
 *  This function stores the nesting depth of critical sections of the current
 *  process (e.g. if a function with a critical section is called from another
 *  critical section) to ensure correct behavior when leaving the section.
 *  This function supports up to 255 nested critical sections.
 */
void os_enterCriticalSection(void) {
    uint8_t GIEB = (SREG & 0b10000000); //Save the state of Global Interrupt Enable Bit
	SREG &= 0b01111111; //Deactivate Global Interrupt Enable Bit
	criticalSectionCount++;
	TIMSK2 &= 0b11111101; //Deactivate Scheduler through deleting OCIE2A Bit
	if(GIEB == 0){
	    SREG &= 0b01111111;
	} else {
	    SREG |= 0b10000000; 
	}; //Restore the state of GIEB
}

/*!
 *  Leaves a critical code section by enabling the scheduler if needed.
 *  This function utilizes the nesting depth of critical sections
 *  stored by os_enterCriticalSection to check if the scheduler
 *  has to be reactivated.
 */
void os_leaveCriticalSection(void) {
    uint8_t GIEB = (SREG & 0b10000000); //Save the state of Global Interrupt Enable Bit
    SREG &= 0b01111111; //Deactivate Global Interrupt Enable Bit
    criticalSectionCount--;
	if(criticalSectionCount == 0) {
		TIMSK2 |= 0b00000010; //Activate Scheduler only when there is no nested critical section
	};
    if(GIEB == 0){
	    SREG &= 0b01111111;
	} else {
	    SREG |= 0b10000000; 
	}; //Restore the state of GIEB
}

/*!
 *  Calculates the checksum of the stack for a certain process.
 *
 *  \param pid The ID of the process for which the stack's checksum has to be calculated.
 *  \return The checksum of the pid'th stack.
 */
StackChecksum os_getStackChecksum(ProcessID pid) {
	uint16_t saveStackPointer = os_processes[pid].sp.as_int; //Save the current address of the stack pointer
	os_processes[pid].sp.as_int = PROCESS_STACK_BOTTOM(pid) - STACK_SIZE_PROC + 1; //Set stack pointer address to top of stack
    StackChecksum newChecksum = os_processes[pid].sp.as_ptr[0]; //The value of the top of stack
	for(uint8_t i = 1; i <= STACK_SIZE_PROC - 1; i++){
		newChecksum ^= os_processes[pid].sp.as_ptr[i]; //XOR everything down to the bottom
	}
	os_processes[pid].sp.as_int = saveStackPointer; //Restore the stack pointer before calculation
	return newChecksum;
}
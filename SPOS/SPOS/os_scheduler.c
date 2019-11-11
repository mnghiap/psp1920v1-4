#include "os_scheduler.h"
#include "util.h"
#include "os_input.h"
#include "os_scheduling_strategies.h"
#include "os_taskman.h"
#include "os_core.h"
#include "string.h"
#include "lcd.h"

#include <avr/interrupt.h>

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
#warning IMPLEMENT STH. HERE

//! Count of currently nested critical sections
#warning IMPLEMENT STH. HERE

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
   
    // 2. Store context and SP of current process 
    saveContext();
    os_processes[os_getCurrentProc()].sp.as_int = SP;
    
    // 3. set SP to scheduler stack
    SP = BOTTOM_OF_ISR_STACK;
    
    // 4. set cur. process to OS_PS_READY
    os_processes[os_getCurrentProc()].state = OS_PS_READY;
    
    // 5. get next process depending on scheduling strategy
    executeScheduler(os_getSchedulingStrategy());
    
    // 6. restore new process (SP and context) and set state
    os_processes[os_getCurrentProc()].state = OS_PS_RUNNING;
    SP = os_processes[os_getCurrentProc()].sp.as_int;
    restoreContext();
    
    // 7. automatic return
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
    #warning IMPLEMENT STH. HERE
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
    // 1. find free space in `os_processes`
    ProcessID pid = 0;
    for (pid = 0; pid <= MAX_NUMBER_OF_PROCESSES; pid++) {
        if (pid == MAX_NUMBER_OF_PROCESSES)  // element that isn't available in `os_programs`
            return INVALID_PROCESS;
        if (os_processes[pid].state == OS_PS_UNUSED)
            break;  // first null elem. found
    }
    
    // 2. load program index
    Program* program_ptr = os_lookupProgramFunction(programID);
    if (program_ptr == NULL)
        return INVALID_PROCESS;
	
	    
    // 3. store program index, state and priority
	os_processes[pid] = (Process){
		.sp = {PROCESS_STACK_BOTTOM(pid) - (32 + 1 + 2)},
        .state = OS_PS_READY,
        .progID = programID,
        .priority = priority
    };
    
	// 4. prepare process stack
    StackPointer ptr = {.as_ptr = (uint8_t*)program_ptr };
    
    os_processes[pid].sp.as_ptr[35 - 0] = (uint8_t)(ptr.as_int & 0xFF);
    os_processes[pid].sp.as_ptr[35 - 1] = (uint8_t)(ptr.as_int >> 8);
    for (int i = 0; i < 33; i++)
        os_processes[pid].sp.as_ptr[35 - i-2] = 0;
        
    return pid;
}

/*!
 *  If all processes have been registered for execution, the OS calls this
 *  function to start the idle program and the concurrent execution of the
 *  applications.
 */
void os_startScheduler(void) {
    #warning IMPLEMENT STH. HERE
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
    #warning IMPLEMENT STH. HERE
}

/*!
 *  This is a getter for retrieving the current scheduling strategy.
 *
 *  \return The current scheduling strategy.
 */
SchedulingStrategy os_getSchedulingStrategy(void) {
    #warning IMPLEMENT STH. HERE
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
    #warning IMPLEMENT STH. HERE
}

/*!
 *  Leaves a critical code section by enabling the scheduler if needed.
 *  This function utilizes the nesting depth of critical sections
 *  stored by os_enterCriticalSection to check if the scheduler
 *  has to be reactivated.
 */
void os_leaveCriticalSection(void) {
    #warning IMPLEMENT STH. HERE
}

/*!
 *  Calculates the checksum of the stack for a certain process.
 *
 *  \param pid The ID of the process for which the stack's checksum has to be calculated.
 *  \return The checksum of the pid'th stack.
 */
StackChecksum os_getStackChecksum(ProcessID pid) {
    #warning IMPLEMENT STH. HERE
}
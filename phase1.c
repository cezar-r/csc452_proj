/*
 Name- Soumay Agarwal
 Course- CSC 452
 Description- This is the phase1a of the assignment, which implements the phase1_init, spork, join,
              quit_phase1a, getpid, dumpProcesses and TEMP_switchTo. This part is responsible for creating processes
              and adding them to the process table. We don't have the dispatcher here, so we rely on TEMP_switchTo
              to context switch to other processes. Using the newHeader to declare the Process struct and define
              different states.

 */

#include "phase1.h"
#include "phase1helper.h"
#include "newHeader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Process processTable[MAXPROC];
int occupiedSlots = 0;

Process *curProcess;

int globalPid = 2;

/**
 * This function is responsible for setting up or initializing our process table.
 * We set up each process in the table with default values which are later manipulated
 * to keep track of actual processes.
 */

void setUpInit()
{
    for (int i = 0; i < MAXPROC; i++)
    {
        processTable[i].name = "";
        processTable[i].pid = 0;
        processTable[i].priority = 0;
        processTable[i].parentPid = -50;
        processTable[i].tableStatus = EMPTY;
        processTable[i].runnableStatus = INITIALIZED;
        processTable[i].stack = NULL;
        processTable[i].runQueueNext = NULL;
        processTable[i].nextSibling = NULL;
        processTable[i].firstChild = NULL;
    }
}

/**
 * This function is responsible for checking for the kernel mode. If any method
 * is called in user mode then we USLOSS_Halt(0).
 *
 * caller- it is the name of the function that calls modeChecker
 */

void modeChecker(char *caller)
{
    if ((USLOSS_PsrGet() & 1) == 0)
    {
        USLOSS_Console("ERROR: Someone attempted to call %s while in user mode!\n", caller);
        USLOSS_Halt(0);
    }
}

/**
 * This function is responsible for calling setUpInit to initialize the process table.
 * It then creates and adds the innit process process in the table at the first slot.
 * This process will run at a priority of 6.
 */

void phase1_init(void)
{
    modeChecker("phase1_init");
    setUpInit();
    // setting up innit process
    processTable[1].name = "init";
    processTable[1].pid = 1;
    processTable[1].priority = 6;
    processTable[1].parentPid = 0;
    processTable[1].runnableStatus = RUNNABLE;
    processTable[1].stack = malloc(USLOSS_MIN_STACK);
    processTable[1].tableStatus = OCCUPIED;
    // curProcess = &processTable[1];
    russ_ContextInit(processTable[1].pid, &(processTable[1].context), processTable[1].stack, USLOSS_MIN_STACK, &init_main, NULL);
    occupiedSlots += 1;
}

/**
 * This function is responsible for creating the child of our current process
 * by adding the new process into the process table and then making the
 * new process the child of the current process.
 * name- the provided name of the process
 * startfunc- the provided function associated with the process
 * arg- the provided arguments of the process
 * stacksize- the provided size of stack of the process
 * priority- the provided priority of the process.
 * return: pid of the created child
 */

int spork(char *name, int (*startFunc)(char *), char *arg,
          int stacksize, int priority)
{
    modeChecker("spork");
    if (stacksize < USLOSS_MIN_STACK)
    {
        return -2;
    }

    if (occupiedSlots == MAXPROC || priority < 1 || priority > 5 || startFunc == NULL || name == NULL || strlen(name) > MAXNAME)
    {
        return -1;
    }

    // checking for the appropriate slot for process insertion
    int slot = globalPid % MAXPROC;
    if (slot == 1)
    {
        globalPid += 1;
        slot = globalPid % MAXPROC;
    }
    while (processTable[slot].tableStatus != EMPTY)
    {
        globalPid += 1;
        slot = globalPid % MAXPROC;
    }

    // setting up the process in the table
    processTable[slot].pid = globalPid;
    processTable[slot].priority = priority;
    processTable[slot].name = name;
    processTable[slot].runnableStatus = RUNNABLE;
    processTable[slot].parentPid = curProcess->pid;
    processTable[slot].stack = malloc(stacksize);
    processTable[slot].tableStatus = OCCUPIED;

    occupiedSlots += 1;
    globalPid += 1;

    russ_ContextInit(processTable[slot].pid, &(processTable[slot].context), processTable[slot].stack, stacksize, startFunc, arg);

    // if the cur process has no child, then this new process
    // becomes the first child
    if (curProcess->firstChild == NULL)
    {
        curProcess->firstChild = &processTable[slot];
    }

    // if the cur process has children, then this new process
    // gets added to the head of the linked list of children.
    else
    {
        Process *tmpProcess = curProcess->firstChild;
        curProcess->firstChild = &processTable[slot];
        processTable[slot].nextSibling = tmpProcess;
    }

    return processTable[slot].pid;
}

/**
 * This function is responsible for joining the dead or terminated process
 * with the parent. The runnable status of the dead child is passed to the status pointer
 * and the pid of the child is returned.
 * status- the out pointer which is given the dead child status
 * return: pid of the dead child
 */

int join(int *status)
{
    modeChecker("join");
    if (status == NULL)
        return -3;

    if (curProcess->firstChild == NULL)
    {
        return -2;
    }

    // Checking for dead children of the curProcess
    Process *genPointer = curProcess->firstChild;
    // prev pointer will hold the dead child
    Process *prev;
    // if the firstchild is dead, pid points to it and the curProcess makes the next sibling
    // the first child
    if (genPointer->endStatus == TERMINATED)
    {
        curProcess->firstChild = curProcess->firstChild->nextSibling;
        prev = genPointer;
    }
    // otherwise we find the dead child by taversing the next siblings
    else
    {
        while (genPointer->nextSibling->endStatus != TERMINATED)
        {
            genPointer = genPointer->nextSibling;
        }
        prev = genPointer->nextSibling;
        genPointer->nextSibling = genPointer->nextSibling->nextSibling;
    }
    int curpid = prev->pid;
    *status = processTable[curpid % MAXPROC].runnableStatus;
    // freeing the child process stack and other fields, so that
    // we can reuse the slot
    free(processTable[curpid % MAXPROC].stack);
    occupiedSlots -= 1;
    processTable[curpid % MAXPROC].name = "";
    processTable[curpid % MAXPROC].pid = 0;
    processTable[curpid % MAXPROC].priority = 0;
    processTable[curpid % MAXPROC].parentPid = -50;
    processTable[curpid % MAXPROC].tableStatus = EMPTY;
    processTable[curpid % MAXPROC].runnableStatus = RUNNABLE;
    processTable[curpid % MAXPROC].stack = NULL;
    processTable[curpid % MAXPROC].runQueueNext = NULL;
    processTable[curpid % MAXPROC].nextSibling = NULL;
    processTable[curpid % MAXPROC].firstChild = NULL;
    return curpid;
}

/**
 * This function is responsible for termination the current process by setting its
 * end status as terminated. The runnable status of the current process is changed to
 * the status passed. Due to the lack of dispatcher we use Temp switch to switch to
 * another process.
 * status- provided status
 * switchToPid- provided pid of process to switch to
 */

void quit_phase_1a(int status, int switchToPid)
{
    modeChecker("quit_phase_1a");
    if (curProcess->firstChild != NULL)
    {
        USLOSS_Console("ERROR: Process pid %d called quit() while it still had children.\n", curProcess->pid);
        USLOSS_Halt(0);
    }
    curProcess->endStatus = TERMINATED;
    curProcess->runnableStatus = status;

    TEMP_switchTo(switchToPid);
}

/**
 * This function is responsible for getting the pid of the current process.
 * return: pid of the current process
 */
int getpid(void)
{
    modeChecker("getpid");
    int holder = curProcess->pid;
    return holder;
}

/**
 * This function is responsible for dumping out information regarding a process
 * through USLOSS_Console.
 */

void dumpProcesses(void)
{
    modeChecker("dumpProcesses");
    USLOSS_Console(" PID  PPID  NAME              PRIORITY  STATE\n");
    for (int j = 0; j < MAXPROC; j++)
    {
        if (processTable[j].tableStatus == OCCUPIED)
        {
            USLOSS_Console("%4d  %4d  %-17s %-10d", processTable[j].pid, processTable[j].parentPid, processTable[j].name, processTable[j].priority);

            if (processTable[j].pid == curProcess->pid)
            {
                USLOSS_Console("Running\n");
            }

            else if (processTable[j].runnableStatus == 0)
            {
                USLOSS_Console("Runnable\n");
            }

            else if (processTable[j].runnableStatus != 0 || processTable[j].runnableStatus != 1)
            {
                USLOSS_Console("Terminated(%d)\n", processTable[j].runnableStatus);
            }
        }
    }
}

/**
 * This function is responsible for context switching to a process whose pid is provided.
 * pid- the provided pid of a process
 */

void TEMP_switchTo(int pid)
{
    modeChecker("TEMP_switchTo");
    if (pid == 1)
    {
        curProcess = &(processTable[1]);
        USLOSS_ContextSwitch(NULL, &(processTable[1].context));
    }
    else
    {
        int slotPosition = pid % MAXPROC;
        Process *temp = curProcess;
        curProcess = &(processTable[slotPosition]);
        USLOSS_ContextSwitch(&(temp->context), &(processTable[slotPosition].context));
    }
}
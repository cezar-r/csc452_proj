/*
 Name- Soumay Agarwal & Cezar Rata
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
processQueue priortyQueues[7];

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
        processTable[i].waitingOnChild = false;
        processTable[i].startTime = 0;
    }
    for (int i = 0; i < 7; i++)
    { // Initialize run list
        priortyQueues[i].head = NULL;
        priortyQueues[i].tail = NULL;
    }
}

void enableInterrupts()
{
    USLOSS_PsrSet(3);
}

void disableInterrupts()
{
    if ((USLOSS_PsrGet() & 1) == 0)
    {
        USLOSS_PsrSet(0);
    }
    else
    {
        USLOSS_PsrSet(1);
    }
}

int getCurTime(void)
{

    int val = currentTime();
    return val;
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
    USLOSS_Console("IN init\n");
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
    priortyQueues[6].head = &processTable[1];
    priortyQueues[6].tail = &processTable[1];
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

void addQueue(Process *proc)
{

    if (priortyQueues[proc->priority].head == NULL)
    {
        priortyQueues[proc->priority].head = priortyQueues[proc->priority].tail = proc;
        proc->runQueueNext = NULL;
    }
    else
    {
        Process *tmp = priortyQueues[proc->priority].tail;
        if (tmp->runQueueNext == NULL)
        {
            tmp->runQueueNext = proc;
        }
        priortyQueues[proc->priority].tail = proc;
    }
}

int spork(char *name, int (*startFunc)(char *), char *arg,
          int stacksize, int priority)
{
    USLOSS_Console("IN SPORK\n");
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

    addQueue(&(processTable[slot]));

    if (strcmp("sentinel", name) != 0 && strcmp("testcase_main", name) != 0)
    {
        disableInterrupts();
        dispatcher();
        enableInterrupts();
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
    USLOSS_Console("IN JOIN\n");
    modeChecker("join");

    if (status == NULL)
    {
        return -3;
    }

    if (curProcess->firstChild == NULL)
    {
        return -2; // No children to wait for
    }

    Process *child = curProcess->firstChild;
    Process *prev = NULL;
    Process *terminatedChild = NULL;

    while (child != NULL)
    {
        if (child->endStatus == TERMINATED)
        {
            terminatedChild = child;
            break;
        }
        prev = child;
        child = child->nextSibling;
    }

    // fIf no terminated child is found, block the current process
    if (terminatedChild == NULL)
    {
        blockMe(WAITING_ON_CHILD);

        // after being unblocked, search for the terminated child again
        child = curProcess->firstChild;
        prev = NULL;
        while (child != NULL)
        {
            if (child->endStatus == TERMINATED)
            {
                terminatedChild = child;
                break;
            }
            prev = child;
            child = child->nextSibling;
        }
    }

    if (terminatedChild == NULL)
    {
        return -2;
    }

    *status = terminatedChild->runnableStatus;

    if (prev == NULL)
    {
        curProcess->firstChild = terminatedChild->nextSibling;
    }
    else
    {
        prev->nextSibling = terminatedChild->nextSibling;
    }

    int terminatedPid = terminatedChild->pid;

    free(terminatedChild->stack);
    terminatedChild->name = "";
    terminatedChild->pid = 0;
    terminatedChild->priority = 0;
    terminatedChild->parentPid = -1;
    terminatedChild->tableStatus = EMPTY;
    terminatedChild->runnableStatus = INITIALIZED;
    terminatedChild->stack = NULL;
    terminatedChild->runQueueNext = NULL;
    terminatedChild->nextSibling = NULL;
    terminatedChild->firstChild = NULL;

    occupiedSlots--;

    return terminatedPid;
}

/**
 * This function is responsible for termination the current process by setting its
 * end status as terminated. The runnable status of the current process is changed to
 * the status passed. It then calls dispatcher to switch processes
 * status- provided status
 */
void quit(int status)
{
    USLOSS_Console("IN QUIT\n");
    modeChecker("quit");

    if (curProcess->firstChild != NULL)
    {
        USLOSS_Console("ERROR: Process pid %d called quit() while it still had children.\n", curProcess->pid);
        USLOSS_Halt(0);
    }

    curProcess->endStatus = TERMINATED;
    curProcess->runnableStatus = status;

    Process *parent = &processTable[curProcess->parentPid % MAXPROC];
    if (parent->runnableStatus == WAITING_ON_CHILD)
    {
        // unblock the parent if it's waiting for a child to terminate
        unblockProc(curProcess->parentPid);
    }

    dispatcher();
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

/*
 * This is a helper function that adds process to the priortyQueues
 * proc: the process being added to the run list.
 */

void removeQueue(Process *proc)
{
    if (priortyQueues[proc->priority].head == proc)
    {
        priortyQueues[proc->priority].head = priortyQueues[proc->priority].head->runQueueNext;
    }
    else
    {
        Process *headNode = priortyQueues[proc->priority].head;
        while (headNode->runQueueNext != proc)
        {
            headNode = headNode->runQueueNext;
        }
        headNode->runQueueNext = headNode->runQueueNext->runQueueNext;
    }
}

void moveBackQueue(Process *proc)
{
    if (priortyQueues[proc->priority].head != priortyQueues[proc->priority].tail)
    {
        Process *tmp1 = priortyQueues[proc->priority].head;
        priortyQueues[proc->priority].head = tmp1->runQueueNext;
        Process *tmp2 = priortyQueues[proc->priority].tail;
        priortyQueues[proc->priority].tail = tmp1;
        tmp2->runQueueNext = tmp1;
    }
}

void dispatcher(void)
{
    // USLOSS_Console("IN DISP\n");
    disableInterrupts();
    if (curProcess == NULL)
    {
        curProcess = &(processTable[1]);
        processTable[1].runnableStatus = RUNNING;
        curProcess->startTime = getCurTime();
        USLOSS_ContextSwitch(NULL, &(processTable[1].context));
    }
    for (int i = 1; i < 7; i++)
    {
        if (priortyQueues[i].head != NULL)
        {
            // USLOSS_Console("Entering here upper cur %d  %d\n", curProcess->priority, priortyQueues[i].head->priority);
            if (curProcess->priority != priortyQueues[i].head->priority)
            {
                // USLOSS_Console("Entering here\n");
                USLOSS_Context oldContext = curProcess->context;
                if (curProcess->runnableStatus == RUNNING && curProcess->endStatus != TERMINATED)
                {
                    curProcess->runnableStatus = RUNNABLE;
                }
                curProcess = priortyQueues[i].head;
                curProcess->runnableStatus = RUNNING;
                curProcess->startTime = getCurTime();
                USLOSS_ContextSwitch(&(oldContext), &(curProcess->context));
                break;
            }
            else
            {
                // if (curProcess->runnableStatus == RUNNING)
                // {
                // USLOSS_Console("Entering here\n");
                if (priortyQueues[i].head->runQueueNext != NULL)
                {
                    USLOSS_Console("Entering here\n");
                    int appropriateTime = currentTime() - priortyQueues[i].head->startTime;
                    if (appropriateTime >= 80000)
                    {
                        USLOSS_Console("Here %d\n", appropriateTime);
                        Process *tempHead = priortyQueues[i].head;
                        tempHead->runnableStatus = RUNNABLE;
                        moveBackQueue(tempHead);
                        curProcess = priortyQueues[i].head;
                        curProcess->runnableStatus = RUNNABLE;
                        USLOSS_ContextSwitch(&(tempHead->context), &(curProcess->context));
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    USLOSS_Console("Entering here\n");
                    break;
                }
                // }
                // else
                // {
                //     break;
                // }
            }
        }
    }

    // Special case when time runs out for program.
    // if (curProcess->runnableStatus == RUNNING && (getCurTime() - curProcess->startTime) >= 80000)
    // {
    //     current->run_state = RUNNABLE;
    //     moveBackQueue();
    //     current->next_proc = NULL;
    //     addQueue(curProcess);
    // }
    enableInterrupts();
}

/**
 * Blocks the current process with the specified newStatus.
 * The process will remain blocked until it is unblocked by unblockProc.
 * The newStatus must be greater than 10.
 *
 * newStatus- The reason for blocking the process.
 */
void blockMe(int newStatus)
{
    USLOSS_Console("IN BLOCK ME\n");
    modeChecker("blockMe");

    if (newStatus <= 10)
    {
        USLOSS_Console("ERROR: blockMe called with invalid newStatus <= 10.\n");
        USLOSS_Halt(0);
    }

    curProcess->runnableStatus = newStatus;
    dispatcher();
}

/**
 * Unblocks the process specified by pid.
 * The process must have been previously blocked by blockMe.
 *
 * pid- The PID of the process to unblock.
 * returns 0 on success, -2 if the process is not blocked or does not exist.
 */
int unblockProc(int pid)
{
    USLOSS_Console("IN UNBLOCK\n");
    modeChecker("unblockProc");

    int slot = pid % MAXPROC;
    Process *proc = &processTable[slot];

    if (proc->pid != pid || proc->runnableStatus <= 10)
    {
        return -2;
    }

    proc->runnableStatus = RUNNABLE;

    addQueue(proc);
    dispatcher();
    return 0;
}

#include <usloss.h>
#define BLOCKED 3
#define RUNNABLE 0
#define RUNNING -21
#define INITIALIZED -60
#define TERMINATED -5
#define EMPTY 10
#define OCCUPIED 20

typedef struct processQueue
{
    Process *head;
    Process *tail;
} processQueue;

// Struct for our process node
typedef struct Process
{
    char *name;
    int pid;
    int priority;
    int parentPid;
    int tableStatus;
    int runnableStatus;
    int endStatus;
    // int reasonStatus;
    // char *arg;
    // int (*startFunc)(char *);
    char *stack;
    USLOSS_Context context;
    boolean waitingOnChild;
    int startTime;
    struct Process *runQueueNext;
    struct Process *firstChild;
    struct Process *nextSibling;
} Process;
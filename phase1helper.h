#ifndef _PHASE_1_HELPER__H__INCLUDED_
#define _PHASE_1_HELPER__H__INCLUDED_

#include "usloss.h"



/* students will call this.  It simplifies the student experience
 * by providing a trampoline which makes it easier to initialize
 * a new process.
 */
extern void russ_ContextInit(int pid, USLOSS_Context *state,
                             char *stack, int stackSize,
                             int (*func)(char*), char *arg);



/* this utility function is useful in Phase 1B, where you need to be
 * aware of how long a process has run.  It returns the elapsed time
 * (since USLOSS began), in microseconds.
 */
extern int currentTime(void);



/* ---- STUDENTS CAN IGNORE EVERYTHING BELOW THIS LINE ---
 *
 * These are functions in the phase1_common_code which are called by the
 * phase1helper, or vice-versa
 */
extern void phase1helper_init(void);
extern void startProcesses(void);
extern void phase1_dispatcher_wrapper(int target_pid);

#endif


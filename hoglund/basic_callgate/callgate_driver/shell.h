#ifndef __shell_h__
#define __shell_h__

// queues command for IRQL_PASSIVE execution
VOID QueueCommand( char * );

VOID StartCommandThread();
VOID KillCommandThread();

#endif
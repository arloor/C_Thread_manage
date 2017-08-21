/*
 ============================================================================
 Name        : OSA1.c
 Author      : Robert Sheehan
 Version     : 1.0
 Description : Single thread implementation.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "littleThread.h"
#include "threads1.c" // rename this for different threads


Thread newThread; // the thread currently being set up
Thread mainThread; // the main thread


struct sigaction setUpAction;

void printStates(Thread threads[],int threadNum){
    printf("Thread States\n=============\n");
    for (int i = 0; i < threadNum; ++i) {
        printf("threadID: %d state:%s\n", threads[i]->tid,states_str[threads[i]->state]  );

    }
    printf("\n");
}

void printStates_1(Thread thread,int threadNum){
    printf("Thread States\n=============\n");
    while(thread->tid!=0){
        thread=thread->next;
    }
    for (int i = 0; i < NUMTHREADS; ++i) {
        printf("threadID: %d state:%s\n", thread->tid,states_str[thread->state]  );
        thread=thread->next;
    }
    printf("\n");
}

/*
 * Switches execution from prevThread to nextThread.
 */
void switcher(Thread prevThread, Thread nextThread,Thread threads[]) {
    int i=0;
    while(i<NUMTHREADS){
        if (prevThread->state == FINISHED) { // it has finished
            printf("\ndisposing %d\n", prevThread->tid);
            //free(prevThread->stackAddr); // Wow!
            printStates_1(prevThread,NUMTHREADS);
            longjmp(nextThread->environment, 1);
        } else if (setjmp(prevThread->environment) == 0) { // so we can come back here
            prevThread->state = READY;
            nextThread->state = RUNNING;
            printf("scheduling %d\n", nextThread->tid);
            //printStates(threads,NUMTHREADS);
            longjmp(nextThread->environment, 1);
        }
        prevThread=nextThread;
        nextThread=nextThread->next;
        i++;
    }
}

/*
 * Associates the signal stack with the newThread.
 * Also sets up the newThread to start running after it is long jumped to.
 * This is called when SIGUSR1 is received.
 */
void associateStack(int signum,Thread threads) {
	Thread localThread = newThread; // what if we don't use this local variable?
	localThread->state = READY; // now it has its stack
	if (setjmp(localThread->environment) != 0) { // will be zero if called directly
		(localThread->start)();
		localThread->state = FINISHED;
		switcher(localThread, mainThread,threads); // at the moment back to the main thread
	}
}

/*
 * Sets up the user signal handler so that when SIGUSR1 is received
 * it will use a separate stack. This stack is then associated with
 * the newThread when the signal handler associateStack is executed.
 */
void setUpStackTransfer() {
	setUpAction.sa_handler = (void *) associateStack;
	setUpAction.sa_flags = SA_ONSTACK;
	sigaction(SIGUSR1, &setUpAction, NULL);
}

/*
 *  Sets up the new thread.
 *  The startFunc is the function called when the thread starts running.
 *  It also allocates space for the thread's stack.
 *  This stack will be the stack used by the SIGUSR1 signal handler.
 */
Thread createThread(void (startFunc)()) {
	static int nextTID = 0;
	Thread thread;
	stack_t threadStack;

	if ((thread = malloc(sizeof(struct thread))) == NULL) {
		perror("allocating thread");
		exit(EXIT_FAILURE);
	}
	thread->tid = nextTID++;
	thread->state = SETUP;
	thread->start = startFunc;
	if ((threadStack.ss_sp = malloc(SIGSTKSZ)) == NULL) { // space for the stack
		perror("allocating stack");
		exit(EXIT_FAILURE);
	}
	thread->stackAddr = threadStack.ss_sp;
	threadStack.ss_size = SIGSTKSZ; // the size of the stack
	threadStack.ss_flags = 0;
	if (sigaltstack(&threadStack, NULL) < 0) { // signal handled on threadStack
		perror("sigaltstack");
		exit(EXIT_FAILURE);
	}
	newThread = thread; // So that the signal handler can find this thread
	kill(getpid(), SIGUSR1); // Send the signal. After this everything is set.
	return thread;
}

int main(void) {
	struct thread controller;
    Thread threads[NUMTHREADS];
	mainThread = &controller;
	mainThread->state = RUNNING;
	setUpStackTransfer();
	// create the threads
	for (int t = 0; t < NUMTHREADS; t++) {
		threads[t] = createThread(threadFuncs[t]);
	}

	for (int i = 0; i <NUMTHREADS ; ++i) {
		threads[i]->next=threads[(i+1)%NUMTHREADS];
		threads[(i+1)%NUMTHREADS]->prev=threads[i];
	}


    printStates(threads,NUMTHREADS);
	puts("switching to first thread");
	switcher(mainThread, threads[0],threads);
	puts("back to the main thread");
    printStates(threads,NUMTHREADS);
	return EXIT_SUCCESS;
}

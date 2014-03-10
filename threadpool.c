/**
 * threadpool.c
 *
 * This file will contain your implementation of a threadpool.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include "threadpool.h"
#include "list.c"

// _threadpool is the internal threadpool structure that is
// cast to type "threadpool" before it given out to callers
typedef struct _threadpool_st {
	pthread_t      *array;
	pthread_mutex_t mutex;
	pthread_cond_t  job_posted; 
	pthread_cond_t  job_taken; 
	poolstate_t     state;
	int             threadCount;
	int             live;
 	queueHead      *job_queue;
} _threadpool;

threadpool create_threadpool(int num_threads_in_pool) {
	_threadpool *pool;

	// sanity check the argument
	if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
	return NULL;

	pool = (_threadpool *) malloc(sizeof(_threadpool));
	if (pool == NULL) {
	fprintf(stderr, "Out of memory creating a new threadpool!\n");
	return NULL;
	}

	// add your code here to initialize the newly created threadpool

	return (threadpool) pool;
}


void dispatch(threadpool from_me, dispatch_fn dispatch_to_here, void *arg) {
	_threadpool *pool = (_threadpool *) from_me;
}

void destroy_threadpool(threadpool destroyme) {
	_threadpool *pool = (_threadpool *) destroyme;
}


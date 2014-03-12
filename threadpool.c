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
#include "queue.c"


// State of the threadpool
typedef enum {
  ALL_RUN, ALL_EXIT
} poolstate;


// _threadpool is the internal threadpool structure that is
// cast to type "threadpool" before it given out to callers
typedef struct _threadpool_st {
	pthread_t      *array;

	pthread_mutex_t mutex; // a lock on the queue
	pthread_cond_t  job_posted; // for when a job is posted
	pthread_cond_t  job_taken; // for when a job is consumed 

	poolstate   state;
	int         threadCount;
	int         live;
	Queue       *q;

} _threadpool;


// Worker(consumer function) run by all the threads
void * work (void * sharedpool) {

}


threadpool create_threadpool(int num_threads_in_pool) {
	_threadpool *pool;

	// sanity check the argument
	if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
	return NULL;
	
	// Create the pool in memory
	pool = (_threadpool *) malloc(sizeof(_threadpool));
	if (pool == NULL) {
	fprintf(stderr, "Out of memory creating a new threadpool!\n");
	return NULL;
	}


	// initialize everything but the array and live thread count
	pthread_mutex_init(&(pool->mutex), NULL);
	pthread_cond_init(&(pool->job_posted), NULL);
	pthread_cond_init(&(pool->job_taken), NULL);
	pool->arrsz = num_threads_in_pool;
	pool->state = ALL_RUN;
	pool->theQueue = makeQueue(num_threads_in_pool);
	gettimeofday(&pool->created, NULL);

	// Make an array of threads
	pool->array = (pthread_t *) malloc (pool->arrsz * sizeof(pthread_t));
	if (NULL == pool->array) {
		fprintf(stderr, "\n\nOut of memory allocating thread array!\n");
		free(pool);
		pool = NULL;
		return NULL;
	}


	// Start each thread in the array
	for (i = 0; i < pool->arrsz; ++i) {
		
		if (0 != pthread_create(pool->array + i, NULL, do_work, (void *) pool)) {
			perror("\n\nThread creation failed:");
			exit(EXIT_FAILURE);
		}

		pool->live = i+1;
		pthread_detach(pool->array[i]);  // automatic cleanup when thread exits.
	}


	return (threadpool) pool;
}


void dispatch(threadpool from_me, dispatch_fn dispatch_to_here, void *arg) {
	_threadpool *pool = (_threadpool *) from_me;
}

void destroy_threadpool(threadpool destroyme) {
	_threadpool *pool = (_threadpool *) destroyme;
}
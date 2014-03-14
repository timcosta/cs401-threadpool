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
  RUNNING, EXITING
} poolstate;


// _threadpool is the internal threadpool structure that is
// cast to type "threadpool" before it given out to callers
typedef struct _threadpool_st {
	pthread_t      *array;

	pthread_mutex_t mutex; // a lock on the queue
	pthread_cond_t  jobPosted; // for when a job is posted
	pthread_cond_t  jobTaken; // for when a job is consumed 

	poolstate   state;
	int         threadCount;
	int         numLive;
	Queue       *q;

} _threadpool;


// Worker(consumer function) run by all the threads
void * work (void * sharedpool) {

	// Create a pointer to the thread pool
	_threadpool *pool = (_threadpool *) sharedpool;

	// Local Vars for new jobs this thread will work on 
	dispatch_fn  myJob;
	void        *myArgs;

	// Obtain a lock on job queue read/write
	if (0 != pthread_mutex_lock(&(pool->mutex))) {
		fprintf(stderr, "nMutex lock failed!\n");
		exit(0);
	}

	// wait for a job 
	do {

		while(!isJobAvailable(pool->q)) {
			pthread_cond_wait(&(pool->jobPosted), &(pool->mutex));
		}

		// If state has changed stop the job loop and prepare to terminate the thread
		if (pool->state == EXITING) break;
	
		// Get a job to do from the queue
		removeJob(pool->q, &myJob, &myArgs);

		// Allow producer to add more jobs to queue
		pthread_cond_signal(&(pool->jobTaken));


		// Yield the mutex lock for producer and other worker threads
		if (0 != pthread_mutex_unlock(&(pool->mutex))) {
			fprintf(stderr, "nMutex unlock failed!\n");
			exit(0);
		}


		/** Execute the job **/
		myJob(myArgs);


		// Re-obtain the lock on job queue read/write
		if (0 != pthread_mutex_lock(&(pool->mutex))) {
			fprintf(stderr, "nMutex lock failed!\n");
			exit(0);
		}


	}  while(1);


	// Decrease the number of live threads
	pool->numLive--;


	// Yield the mutex lock
	if (0 != pthread_mutex_unlock(&(pool->mutex))) {
		fprintf(stderr, "nMutex unlock failed!\n");
		exit(0);
	}

	printf("Thread Dying!...");
	return NULL;
}


// Create a new threadpool to do work
threadpool create_threadpool(int num_threads_in_pool) {
	_threadpool *pool;

	// sanity check the argument
	if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL)) return NULL;
	
	// Create the pool in memory
	pool = (_threadpool *) malloc(sizeof(_threadpool));
	if (pool == NULL) {
		fprintf(stderr, "\n\nOut of memory creating a new threadpool!\n");
		return NULL;
	}

	// initialize mutexes, cond vars and pool data 
	pthread_mutex_init(&(pool->mutex), NULL);
	pthread_cond_init(&(pool->jobPosted), NULL);
	pthread_cond_init(&(pool->jobTaken), NULL);
	pool->threadCount = num_threads_in_pool;
	pool->state = RUNNING;
	pool->numLive = 0;
	pool->q = makeQueue();

	// Make an array of threads
	pool->array = (pthread_t *) malloc (pool->threadCount * sizeof(pthread_t));
	if (NULL == pool->array) {
		fprintf(stderr, "\n\nOut of memory allocating thread array!\n");
		free(pool);
		pool = NULL;
		return NULL;
	}

	// Start each thread in the array
	for (int i = 0; i < pool->threadCount; i++) {
		if (0 != pthread_create(pool->array[i], NULL, work, (void *) pool)) {
			fprintf(stderr, "\n\nThread creation failed:\n");
			exit(0);
		}

		pool->numLive++;
		pthread_detach(pool->array[i]);  // Release thread memory when thread exits
	}


	return (threadpool) pool;
}


// Allocate a new job to the Threadpool
void dispatch(threadpool from_me, dispatch_fn dispatch_to_here, void *arg) {
	_threadpool *pool = (_threadpool *) from_me;
	if(pool != (_threadpool *) arg){
		pthread_cleanup_push(pthread_mutex_unlock, (void *) &pool->mutex);

		if (pthread_mutex_lock(&pool->mutex) != 0) {
			perror("Mutex lock fail");
			exit(-1);
		}

		while(!canAcceptWork(pool->q)) {
			pthread_cond_signal(&pool->jobPosted);
			pthread_cond_wait(&pool->jobTaken,&pool->mutex);
		}

		addWorkOrder(pool->q,dispatch_to_here,arg,NULL,NULL);

		pthread_cond_signal(&pool->jobPosted);

		if (0 != pthread_mutex_unlock(&pool->mutex)) {
			perror("\n\nMutex unlock failed!:");
			exit(EXIT_FAILURE);
		}
		pthread_cleanup_pop(1);
	}
}

// Shut Down the Thread Pool
void destroy_threadpool(threadpool destroyme) {
	_threadpool *pool = (_threadpool *) destroyme;

	int oldType;
	pthread_setcanceltype(PTHREAD_CANCELLED_DEFERRED,&oldType);
	pthread_cleanup_push(pthread_mutex_unlock, (void*) &pool->mutex);

	if(pthread_mutex_lock(&pool->mutex) != 0) {
		perror("Failed to lock mutext");
		exit(-1);
	}

	pool->state = ALL_EXIT;
	while (pool->numLive > 0) {
		pthread_cond_signal(&pool->jobPosted);
		pthread_cond_wait(&pool->jobTaken, &pool->mutex);
	}
	memset(pool->array,0,pool->threadCount * sizeof(pthread_t));
	free(pool->array);
	pthread_cleanup_pop(0);

	if (pthread_mutex_unlock(&pool->mutex) != 0) {
		perror("\n\nFailed to unlock mutex");
		exit(EXIT_FAILURE);
	}

	if (pthread_mutex_destroy(&pool->mutex) != 0) {
		perror("\nFailed to destroy mutex");
		exit(EXIT_FAILURE);
	}

	if (pthread_cond_destroy(&pool->jobPosted) != 0) {
		perror("\nFailed to destroy jobPosted");
		exit(EXIT_FAILURE);
	}

	if (pthread_cond_destroy(&pool->jobTaken) != 0) {
		perror("\nFailed to destroy jobTaken");
		exit(EXIT_FAILURE);
	}

	memset(pool, 0, sizeof(_threadpool));

	free(pool);
	pool = NULL;
	destroyme = NULL;

}
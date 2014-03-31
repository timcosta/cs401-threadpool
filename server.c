/**
 * server.c, copyright 2001 Steve Gribble
 *
 * The server is a multi-threaded program.  First, it opens
 * up a "listening socket" so that clients can connect to
 * it.  Then, it enters a loop; in each iteration, it
 * accepts a new connection from the client, and passes the connection 
 * to a thread from a thread pool to handle the request. The thread
 * reads a request, computes for a while, sends a response, 
 * then closes the connection.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include "SocketLibrary/socklib.h"
#include "common.h"
#include "threadpool.h"

extern int errno;

int   setup_listen(char *socketNumber);
char *read_request(int fd);
char *process_request(char *request, int *response_length, int *num_loops);

// This is dispatched to thread pool. Handles connection read, process, and write. 
char *handle_request(void * rargs); 

void  send_response(int fd, char *response, int response_length);

// Struct that stores the arguments to be passed to handle_request() 
typedef struct reqArgs {
  int socket;     /* socket to communicate on */
  int numLoops;  /* number of loops(of work) for process request to do*/
} reqArgs;

/**
 * This program should be invoked as ./server <socketnumber> <poolsize> <num_loops>", for
 * example, "./server 4342 16 100".
 */

int main(int argc, char **argv)
{
  char buf[1000];
  int  socket_listen;
  int  socket_talk;
  int  poolSize; // # of threads in the pool
  int  numLoops; // num loops for worl
  threadpool pool; // the threadpool
  reqArgs rargs;
  clock_t begin, end;
  int dispCount = 0;

  // Check for the correct number of args
  if (argc != 4)
  {
    fprintf(stderr, "(SERVER): Invoke as  './server <socketnumber> <poolsize> <num_loops>'\n");
    fprintf(stderr, "(SERVER): for example, './server 4342 16 100'\n");
    exit(-1);
  }

  /* 
   * Set up the 'listening socket'.  This establishes a network
   * IP_address:port_number that other programs can connect with.
   */
  socket_listen = setup_listen(argv[1]);

  poolSize = atoi(argv[2]); // Get the # of threads in the pool

  numLoops = atoi(argv[3]); // Get the # of loops of work

  // Create the threadpool with the appropriate number of threads
  pool = create_threadpool(poolSize);

  /* 
   * Here's the main loop of our program.  Inside the loop, a
   *  connection is listened for, and then dispatches threads to handle new requests. 
   */
  begin = clock();
  while(((double)(end - begin) / CLOCKS_PER_SEC) < 60.0) {
    //if(dispCount == 9000) begin = clock();
    //if (((int)(end-begin)/CLOCKS_PER_SEC)%10 == 0) fprintf(stdout,"Dispatched %d in %f sec\n",dispCount,(double)(end-begin)/CLOCKS_PER_SEC);
    socket_talk = saccept(socket_listen);  // step 1
    //fprintf(stdout,"accepted connection\n");
    if (socket_talk < 0) {
      fprintf(stderr, "An error occured in the server; a connection\n");
      fprintf(stderr, "failed because of ");
      perror("");
      exit(1);
    }

    // Set up args for handling request
    rargs.socket = socket_talk;
    rargs.numLoops = numLoops;

    // Let thread from threadpool handle the request
    dispatch(pool, handle_request, &rargs);

    // Get the clock val after dispatch
    end = clock();
    dispCount++;
    //fprintf(stdout,"just dispatched\n");
  }

  fprintf(stdout,"There are %f tasks/second for %d threads and %d loops\n", (double)(dispCount)/60.0, poolSize, numLoops);
  exit(0);
}



/**
 * This function accepts a string of the form "5654", and opens up
 * a listening socket on the port associated with that string.  In
 * case of error, this function simply bonks out.
 */

int setup_listen(char *socketNumber) {
  int socket_listen;

  if ((socket_listen = slisten(socketNumber)) < 0) {
    perror("(SERVER): slisten");
    exit(1);
  }

  return socket_listen;
}


// This is dispatched to thread pool. Handles connection read, process, and write. 
char *handle_request(void * rargs) { 

  // Get the arguments(port, and # of loops)
  struct reqArgs *tempArgs = (struct reqArgs *) rargs;
  int    socket_talk = tempArgs->socket;
  int    num_loops = tempArgs->numLoops;

  // Vars to store the request and response
  char *request = NULL;
  char *response = NULL;


  request = read_request(socket_talk);  // Read the request
    if (request != NULL) {
      int response_length;

      response = process_request(request, &response_length, &num_loops);  // Process the request(Do work)

      if (response != NULL) {
  send_response(socket_talk, response, response_length);  // Write response back to client
      }
    }
    close(socket_talk);  // Close the connection

    // clean up allocated memory, if any
    if (request != NULL)
      free(request);
    if (response != NULL)
      free(response);

}


/**
 * This function reads a request off of the given socket.
 * This function is thread-safe.
 */

char *read_request(int fd) {
  char *request = (char *) malloc(REQUEST_SIZE*sizeof(char));
  int   ret;

  if (request == NULL) {
    fprintf(stderr, "(SERVER): out of memory!\n");
    exit(-1);
  }

  ret = correct_read(fd, request, REQUEST_SIZE);
  if (ret != REQUEST_SIZE) {
    free(request);
    request = NULL;
  }
  return request;
}

/**
 * This function crunches on a request, returning a response.
 * This is where all of the hard work happens.  
 * This function is thread-safe.
 */


char *process_request(char *request, int *response_length, int *num_loops) {
  char *response = (char *) malloc(RESPONSE_SIZE*sizeof(char));
  int numLoops = *num_loops;
  int   i,j;

  // just do some mindless character munging here

  for (i=0; i<RESPONSE_SIZE; i++)
    response[i] = request[i%REQUEST_SIZE];

  for (j=0; j<numLoops; j++) {
    for (i=0; i<RESPONSE_SIZE; i++) {
      char swap;

      swap = response[((i+1)%RESPONSE_SIZE)];
      response[((i+1)%RESPONSE_SIZE)] = response[i];
      response[i] = swap;
    }
  }
  *response_length = RESPONSE_SIZE;
  return response;
}


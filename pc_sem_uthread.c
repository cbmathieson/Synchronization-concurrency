#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"
#include "uthread_sem.h"

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

uthread_sem_t mutex, empty, full;

int items = 0;

void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO
	  uthread_sem_wait(empty);
	  uthread_sem_wait(mutex);

	  items++;
	  histogram[items]++;

	  uthread_sem_signal(mutex);
	  uthread_sem_signal(full);
  }
  return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO
	  uthread_sem_wait(full);
	  uthread_sem_wait(mutex);

	  items--;
	  histogram[items]++;

	  uthread_sem_signal(mutex);
	  uthread_sem_signal(empty);

  }
  return NULL;
}

int main (int argc, char** argv) {
  uthread_t t[4];

  uthread_init (4);

  // TODO: Create Threads and Join
  mutex = uthread_sem_create(1);
  full = uthread_sem_create(0);
  empty = uthread_sem_create(MAX_ITEMS);

    uthread_t producer1 = uthread_create(producer, NULL);
    uthread_t consumer1 = uthread_create(consumer, NULL);
    uthread_t producer2 = uthread_create(producer, NULL);
    uthread_t consumer2 = uthread_create(consumer, NULL);
    
    if(uthread_join(producer1, NULL)) {
        fprintf(stderr, "Failed to join thread");
        return 2;
    }
    if(uthread_join(consumer1, NULL)) {
        fprintf(stderr, "Failed to join thread");
        return 2;
    }
    if(uthread_join(producer2, NULL)) {
        fprintf(stderr, "Failed to join thread");
        return 2;
    }
    if(uthread_join(consumer2, NULL)) {
        fprintf(stderr, "Failed to join thread");
        return 2;
    }

    //destroy semaphores

    uthread_sem_destroy(mutex);
    uthread_sem_destroy(full);
    uthread_sem_destroy(empty);

  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (t) / sizeof (uthread_t) * NUM_ITERATIONS);

}

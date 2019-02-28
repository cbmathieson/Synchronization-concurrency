#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"
#include "spinlock.h"

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

int items = 0;

uthread_mutex_t mutex;
uthread_cond_t full_cond;
uthread_cond_t empty_cond;

void* producer (void* v) {
    for (int i=0; i<NUM_ITERATIONS; i++) {
        // TODO
        uthread_mutex_lock(mutex);
        
        while(items >= MAX_ITEMS) {
            producer_wait_count++;
            uthread_cond_wait(full_cond);
        }
        
        if(items < MAX_ITEMS) {
            items++;
            histogram[items]++;
        } else {
            i--;
        }
        uthread_cond_signal(empty_cond);
        uthread_mutex_unlock(mutex);
    }
    return NULL;
}

void* consumer (void* v) {
    for (int i=0; i<NUM_ITERATIONS; i++) {
        // TODO
        
        uthread_mutex_lock(mutex);

        while(items < 1) {
            consumer_wait_count++;
            uthread_cond_wait(empty_cond);
        }
        
        if(items > 0) {
            items--;
            histogram[items]++;
        } else {
            i--;
        }
        uthread_cond_signal(full_cond);
        uthread_mutex_unlock(mutex);
    }
    return NULL;
}

int main (int argc, char** argv) {
    uthread_t t[4];
    
    uthread_init (4);
    
    mutex = uthread_mutex_create();
    full_cond = uthread_cond_create(mutex);
    empty_cond = uthread_cond_create(mutex);
    
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
    
    printf ("producer_wait_count=%d\nconsumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);
    printf ("items value histogram:\n");
    int sum=0;
    for (int i = 0; i <= MAX_ITEMS; i++) {
        printf ("  items=%d, %d times\n", i, histogram [i]);
        sum += histogram [i];
    }
    assert (sum == sizeof (t) / sizeof (uthread_t) * NUM_ITERATIONS);
}

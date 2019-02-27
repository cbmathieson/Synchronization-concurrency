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

int producer_wait_count = 0;     // # of times producer had to wait
int consumer_wait_count = 0;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

int items = 0;
spinlock_t lock;

void* producer (void* v) {
    for (int i=0; i<NUM_ITERATIONS; i++) {
        // TODO
        spinlock_lock(&lock);
        
        if(items >= MAX_ITEMS) {
            spinlock_unlock(&lock);
            while(items >= MAX_ITEMS){
                producer_wait_count++;
            }
            spinlock_lock(&lock);
            if(items < MAX_ITEMS) {
                items++;
                histogram[items]++;
                spinlock_unlock(&lock);
            } else {
                i--;
                producer_wait_count++;
                spinlock_unlock(&lock);
            }
        } else {
            items++;
            histogram[items]++;
            spinlock_unlock(&lock);
        }
    }
    return NULL;
}

void* consumer (void* v) {
    for (int i=0; i<NUM_ITERATIONS; i++) {
        // TODO
        
        spinlock_lock(&lock);
        
        if(items < 1) {
            spinlock_unlock(&lock);
            while(items < 1){
                consumer_wait_count++;
            }
            spinlock_lock(&lock);
            if(items > 0) {
                items--;
                histogram[items]++;
                spinlock_unlock(&lock);
            } else {
                i--;
                consumer_wait_count++;
                spinlock_unlock(&lock);
            }
        } else {
            items--;
            histogram[items]++;
            spinlock_unlock(&lock);
        }
    }
    return NULL;
}

int main (int argc, char** argv) {
    uthread_t t[4];
    
    uthread_init (4);
    
    // TODO: Create Threads and Join
    spinlock_create(&lock);
    
    uthread_t consumer1 = uthread_create(producer, NULL);
    uthread_t consumer2 = uthread_create(producer, NULL);
    uthread_t producer1 = uthread_create(consumer, NULL);
    uthread_t producer2 = uthread_create(consumer, NULL);
    
    if(uthread_join(consumer1, NULL)) {
        fprintf(stderr, "Failed to join thread");
        return 2;
    }
    if(uthread_join(consumer2, NULL)) {
        fprintf(stderr, "Failed to join thread");
        return 2;
    }
    if(uthread_join(producer1, NULL)) {
        fprintf(stderr, "Failed to join thread");
        return 2;
    }
    if(uthread_join(producer2, NULL)) {
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

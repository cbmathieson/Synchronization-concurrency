#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS = 2;
const int NUM_PRODUCERS = 2;


int hist [MAX_ITEMS+1];

int items = 0;

sem_t mutex;
sem_t empty, full;

void* producer (void *v) {

	for(int i=0; i<NUM_ITERATIONS; i++) {
	
		sem_wait(&empty);
		sem_wait(&mutex);

		items++;
		hist[items]++;

		sem_post(&mutex);

		sem_post(&full);
	}
	return NULL;
}

void* consumer (void *v) {

	for(int i = 0; i<NUM_ITERATIONS; i++) {

		sem_wait(&full);
		sem_wait(&mutex);

		items--;
		hist[items]++;
		sem_post(&mutex);

		sem_post(&empty);

	}
	return NULL;
}

int main() {
	
	printf("beginning...\n");
	sem_init(&mutex, 0, 1);
	sem_init(&full, 0, 0);
	sem_init(&empty, 0, MAX_ITEMS);

	pthread_t producer1;
	pthread_t producer2;
	pthread_t consumer1;
	pthread_t consumer2;
	
	//Create threads

	if(pthread_create(&producer1, NULL, producer, NULL)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}
	if(pthread_create(&consumer1, NULL, consumer, NULL)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	} 
	if(pthread_create(&producer2, NULL, producer, NULL)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}
	if(pthread_create(&consumer2, NULL, consumer, NULL)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}

	//wait for threads to finish
	if(pthread_join(producer1, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return 2;
	}
	if(pthread_join(consumer1, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return 2;
	}
	if(pthread_join(producer2, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return 2;
	}
	if(pthread_join(consumer2, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return 2;
	}

	printf("item occurence count:\n");
	int sum = 0;
	for(int i = 0; i <= MAX_ITEMS; i++) {
		printf("%d: %d\n", i, hist[i]);
		sum += hist[i];
	}

	assert (sum == NUM_ITERATIONS);
	return 0;
}

		
	

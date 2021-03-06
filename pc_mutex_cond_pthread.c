#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_ITEMS 10

pthread_cond_t empty_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t full_condition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int producer_wait_count = 0;
int consumer_wait_count = 0;
int hist [11];

int items = 0;

void* producer (void *v) {

	for(int i=0; i<200; i++) {

		pthread_mutex_lock(&mutex);

		while(items >= MAX_ITEMS) {
			producer_wait_count++;
			pthread_cond_wait(&full_condition,&mutex);
		}

		if(items < MAX_ITEMS) {
			items++;
			hist[items]++;
		} else {
			i--;
		}

		pthread_cond_signal(&empty_condition);
		pthread_mutex_unlock(&mutex);

	}
	return NULL;
}

void* consumer (void *v) {

	for(int i = 0; i<200; i++) {

		pthread_mutex_lock(&mutex);

		while(items < 1) {
			consumer_wait_count++;
			pthread_cond_wait(&empty_condition,&mutex);
		}

		if(items > 0) {
			items--;
			hist[items]++;
		} else {
			i--;
		}

		pthread_cond_signal(&full_condition);
		pthread_mutex_unlock(&mutex);
	}
	return NULL;
}

int main() {
	
	printf("beginning...");

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

	printf("producer wait count: %d\nconsumer wait count: %d\n", producer_wait_count, consumer_wait_count);
	printf("item occurence count:\n");
	int sum = 0;
	for(int i = 0; i <= MAX_ITEMS; i++) {
		printf("%d: %d\n", i, hist[i]);
		sum += hist[i];
	}

	printf("sum = %d\n", sum);
	return 0;

}

		
	

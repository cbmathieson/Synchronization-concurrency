#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define NUM_ITERATIONS 1000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

struct Agent {
  pthread_mutex_t mutex;
  pthread_cond_t  match;
  pthread_cond_t  paper;
  pthread_cond_t  tobacco;
  pthread_cond_t  smoke;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  pthread_mutex_init(&agent->mutex, NULL);
  pthread_cond_init(&agent->match, NULL);
  pthread_cond_init(&agent->paper, NULL);
  pthread_cond_init(&agent->tobacco, NULL);
  pthread_cond_init(&agent->smoke, NULL);

  return agent;
};

//Keep track of what resources have become available from the agent
struct Resources {
	int match;
	int paper;
	int tobacco;
};

struct Resources resources = { 0, 0, 0 };

pthread_cond_t match_paper;
pthread_cond_t paper_tobacco;
pthread_cond_t match_tobacco;


/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked

/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random reasources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent_t (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};
	
  //added a 50ms delay so agent couldnt outpace the listeners. Was only happening on macOS but just to be safe!
  nanosleep((const struct timespec[]){{0,50000000L}}, NULL);
  
  pthread_mutex_lock (&a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        pthread_cond_signal (&a->match);
      }
      if (c & PAPER) {
        pthread_cond_signal (&a->paper);
      }
      if (c & TOBACCO) {
        pthread_cond_signal (&a->tobacco);
      }
      pthread_cond_wait (&a->smoke, &a->mutex);
    }
  pthread_mutex_unlock (&a->mutex);
  return NULL;
}

/*---------------------- Call to Action --------------------------*/

void check_resources() {

	
	if(resources.match && resources.paper){
		pthread_cond_signal(&match_paper);
	} else if(resources.match && resources.tobacco){
		pthread_cond_signal(&match_tobacco);
	} else if(resources.paper && resources.tobacco) {
		pthread_cond_signal(&paper_tobacco);
	} else {
		return;
	}

	resources.match = 0;
	resources.paper = 0;
	resources.tobacco = 0;
}
/*------------------------- Listeners ----------------------------*/

void* match_watcher_t (void* av) {
	struct Agent* v = av;
	//try to lock mutex
	pthread_mutex_lock(&v->mutex);
	//poll for Agent to give a match
	for(;;) {
		pthread_cond_wait(&v->match, &v->mutex);
		resources.match = 1;
		check_resources();
	}	
	//unlock mutex
	pthread_mutex_unlock(&v->mutex);
	return NULL;
}

void* paper_watcher_t (void* av) {
	struct Agent* v = av;
	//try to lock mutex
	pthread_mutex_lock(&v->mutex);
	//poll for Agent to give paper
	for(;;) {
		pthread_cond_wait(&v->paper, &v->mutex);
		resources.paper = 1;
		check_resources();
	}
	//unlock mutex
	pthread_mutex_unlock(&v->mutex);
	return NULL;
}

void* tobacco_watcher_t (void* av) {
	struct Agent* v = av;
	//try to lock mutex
	pthread_mutex_lock(&v->mutex);
	//poll for Agent to give tobacco
	for(;;) {
		pthread_cond_wait(&v->tobacco, &v->mutex);
		resources.tobacco = 1;
		check_resources();
	}
	//unlock mutex
	pthread_mutex_unlock(&v->mutex);
	return NULL;
}

/*-------------------------- Smokers -----------------------------*/

void* match_t (void* av) {
	struct Agent* v = av;
	pthread_mutex_lock(&v->mutex);
	for(;;) {
		pthread_cond_wait(&paper_tobacco, &v->mutex);
		pthread_cond_signal(&v->smoke);
		smoke_count [MATCH]++;
	}
	pthread_mutex_unlock(&v->mutex);
	return NULL;
}

void* paper_t (void* av) {
	struct Agent* v = av;
	pthread_mutex_lock(&v->mutex);
	for(;;) {
		pthread_cond_wait(&match_tobacco, &v->mutex);
		pthread_cond_signal(&v->smoke);
		smoke_count [PAPER]++;
	}
	pthread_mutex_unlock(&v->mutex);
	return NULL;
}

void* wacki_tobacci_t (void* av) {
	struct Agent* v = av;
	pthread_mutex_lock(&v->mutex);
	for(;;) {
		pthread_cond_wait(&match_paper, &v->mutex);
		pthread_cond_signal(&v->smoke);
		smoke_count [TOBACCO]++;
	}
	pthread_mutex_unlock(&v->mutex);
	return NULL;	
}

/*----------------------------------------------------------------*/

int main (int argc, char** argv) {
  //declare pthreads
  pthread_t match;
  pthread_t paper;
  pthread_t tobacco;
  pthread_t match_watcher;
  pthread_t paper_watcher;
  pthread_t tobacco_watcher;
  pthread_t agent;
	
  struct Agent*  a = createAgent();
  
  //initialize condition variables
  pthread_cond_init(&match_paper, NULL);
  pthread_cond_init(&match_tobacco, NULL);
  pthread_cond_init(&paper_tobacco, NULL);
  
  //create threads
  pthread_create(&match, NULL, match_t, a);
  pthread_create(&paper, NULL, paper_t, a);
  pthread_create(&tobacco, NULL, wacki_tobacci_t, a);

  pthread_create(&match_watcher, NULL, match_watcher_t, a);
  pthread_create(&paper_watcher, NULL, paper_watcher_t, a);
  pthread_create(&tobacco_watcher, NULL, tobacco_watcher_t, a);

  pthread_create(&agent, NULL, agent_t, a);

  pthread_join (agent, NULL);
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);

  pthread_cond_destroy(&match_paper);
  pthread_cond_destroy(&match_tobacco);
  pthread_cond_destroy(&paper_tobacco);

  pthread_cond_destroy(&a->paper);
  pthread_cond_destroy(&a->match);
  pthread_cond_destroy(&a->tobacco);
  pthread_cond_destroy(&a->smoke);

  pthread_mutex_destroy(&a->mutex);
}

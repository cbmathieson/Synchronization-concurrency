#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"
#include <time.h>

#define NUM_ITERATIONS 1000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

struct Agent {
  uthread_mutex_t mutex;
  uthread_cond_t  match;
  uthread_cond_t  paper;
  uthread_cond_t  tobacco;
  uthread_cond_t  smoke;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  agent->mutex   = uthread_mutex_create();
  agent->paper   = uthread_cond_create (agent->mutex);
  agent->match   = uthread_cond_create (agent->mutex);
  agent->tobacco = uthread_cond_create (agent->mutex);
  agent->smoke   = uthread_cond_create (agent->mutex);
  return agent;
};

//Keep track of what resources have become available from the agent
struct Resources {
	int match;
	int paper;
	int tobacco;
};

struct Resources resources = { 0, 0, 0 };

uthread_cond_t match_paper;
uthread_cond_t paper_tobacco;
uthread_cond_t match_tobacco;

//
// TODO
// You will probably need to add some procedures and struct etc.
//

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
void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};
	
  //added a 50ms delay so agent couldnt outpace the listeners. Was only happening on macOS but just to be safe!
  nanosleep((const struct timespec[]){{0,50000000L}}, NULL);
  
  uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        uthread_cond_signal (a->match);
      }
      if (c & PAPER) {
        uthread_cond_signal (a->paper);
      }
      if (c & TOBACCO) {
        uthread_cond_signal (a->tobacco);
      }
      uthread_cond_wait (a->smoke);
    }
  uthread_mutex_unlock (a->mutex);
  return NULL;
}

/*---------------------- Call to Action --------------------------*/

void check_resources() {

	
	if(resources.match && resources.paper){
		uthread_cond_signal(match_paper);
	} else if(resources.match && resources.tobacco){
		uthread_cond_signal(match_tobacco);
	} else if(resources.paper && resources.tobacco) {
		uthread_cond_signal(paper_tobacco);
	} else {
		return;
	}

	resources.match = 0;
	resources.paper = 0;
	resources.tobacco = 0;
}
/*------------------------- Listeners ----------------------------*/

void* match_watcher (void* av) {
	struct Agent* v = av;
	//try to lock mutex
	uthread_mutex_lock(v->mutex);
	//poll for Agent to give a match
	for(;;) {
		uthread_cond_wait(v->match);
		resources.match = 1;
		check_resources();
	}	
	//unlock mutex
	uthread_mutex_unlock(v->mutex);
}

void* paper_watcher (void* av) {
	struct Agent* v = av;
	//try to lock mutex
	uthread_mutex_lock(v->mutex);
	//poll for Agent to give paper
	for(;;) {
		uthread_cond_wait(v->paper);
		resources.paper = 1;
		check_resources();
	}
	//unlock mutex
	uthread_mutex_unlock(v->mutex);
}

void* tobacco_watcher (void* av) {
	struct Agent* v = av;
	//try to lock mutex
	uthread_mutex_lock(v->mutex);
	//poll for Agent to give tobacco
	for(;;) {
		uthread_cond_wait(v->tobacco);
		resources.tobacco = 1;
		check_resources();
	}
	//unlock mutex
	uthread_mutex_unlock(v->mutex);
}

/*-------------------------- Smokers -----------------------------*/

void* match (void* av) {
	struct Agent* v = av;
	uthread_mutex_lock(v->mutex);
	for(;;) {
		uthread_cond_wait(paper_tobacco);
		uthread_cond_signal(v->smoke);
		smoke_count [MATCH]++;
	}
	uthread_mutex_unlock(v->mutex);
}

void* paper (void* av) {
	struct Agent* v = av;
	uthread_mutex_lock(v->mutex);
	for(;;) {
		uthread_cond_wait(match_tobacco);
		uthread_cond_signal(v->smoke);
		smoke_count [PAPER]++;
	}
	uthread_mutex_unlock(v->mutex);

}

void* wacki_tobacci (void* av) {
	struct Agent* v = av;
	uthread_mutex_lock(v->mutex);
	for(;;) {
		uthread_cond_wait(match_paper);
		uthread_cond_signal(v->smoke);
		smoke_count [TOBACCO]++;
	}
	uthread_mutex_unlock(v->mutex);	
}

/*----------------------------------------------------------------*/

int main (int argc, char** argv) {
  uthread_init (7);
  struct Agent*  a = createAgent();
  // TODO
  
  //initialize condition variables
  match_paper = uthread_cond_create(a->mutex);
  match_tobacco = uthread_cond_create(a->mutex);
  paper_tobacco = uthread_cond_create(a->mutex);
  
  //create threads
  uthread_create(match, a);
  uthread_create(paper, a);
  uthread_create(wacki_tobacci, a);

  uthread_create(match_watcher, a);
  uthread_create(paper_watcher, a);
  uthread_create(tobacco_watcher, a);

  uthread_join (uthread_create (agent, a), 0);
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}

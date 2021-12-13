#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

static int nthread = 1;
static int round = 0;

struct barrier {
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread;      // Number of threads that have reached this round of the barrier
  int round;     // Barrier round
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
}

static void 
barrier()
{
  // YOUR CODE HERE
  //
  // Block until all threads have called barrier() and
  // then increment bstate.round.
  //

  
  //"each thread blocks in barrier until all nthreads have called barrier()"
  //
  //You will need:
  //
  //0. the lock primitives you have seen in the ph assignment
  //
  //1. pthread_cond_wait(&cond, &mutex)
  //  "go to sleep on cond, releasing lock mutex, acquiring upon wakeup"
  //  "releases the mutex when called and re-acquires the mutex before returning"
  //
  //2. pthread_cond_broadcast(&cond)
  //  "wake up every thread sleeping on cond"
  //
  //struct is called bstate not barrier

  //turn on the lock (from ph)
  //pthread_mutex_lock(bstate.barrier_mutex);
  pthread_mutex_lock(&(bstate.barrier_mutex));

  //increment nthread manually
  bstate.nthread += 1;

  //case 1: check if all the threads have arrived
  if (bstate.nthread == nthread) //nthread comes from argv[1]
  {
    //reset the number of threads
    bstate.nthread = 0;
    //increment the round counter
    bstate.round += 1;

    //send the wakeup call
    pthread_cond_broadcast(&(bstate.barrier_cond));
  }
  
  //case 2: put the thread to sleep and wait
  else
  {
    pthread_cond_wait(&(bstate.barrier_cond), &(bstate.barrier_mutex));
  }


  //I suppose i should unlock it too
  //unlock from ph
  pthread_mutex_unlock(&(bstate.barrier_mutex));

  



  



  // END CODE HERE
  
}

static void *
thread(void *xa)
{
  long n = (long) xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++) {
    int t = bstate.round;
    assert (i == t);
    barrier();
    usleep(random() % 100);
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}

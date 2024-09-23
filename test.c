#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
// thread function
void *thread_fn(void *arg)
{
  int id = (int)arg;
  printf("thread runs (%d)\n", id);
  sleep(id); // wait for id second
  id *= 1000;
  printf("terminate thread (%d)\n", id);
  return (void *)(id);
}

int main()
{
  pthread_t t1, t2;
  int retval;
  pthread_create(&t1, NULL, thread_fn, (void *)1);
  pthread_create(&t2, NULL, thread_fn, (void *)2);
  pthread_join(t1, (void *)&retval);
  printf("thread join: %d\n", retval);
  pthread_join(t2, (void *)&retval);
  printf("thread join: %d\n", retval);
  return 0;
}
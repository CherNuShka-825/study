#include "mythread.h"
#include <sched.h>
#include <stdio.h>

void *worker(void *arg) {
  int *result = arg;
  __atomic_store_n(result, 42, __ATOMIC_RELAXED);
  return NULL;
}

int main(void) {
  mythread_t thread;
  int result = 0;

  if (mythread_create(&thread, worker, &result) == -1) {
    perror("mythread_create");
    return 1;
  }

  while (__atomic_load_n(&result, __ATOMIC_RELAXED) == 0) {
    sched_yield();
  }

  printf("Result: %d\n", result);

  return 0;
}

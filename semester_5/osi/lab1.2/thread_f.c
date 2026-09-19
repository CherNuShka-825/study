#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *arg) {
  printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(),
         gettid());
  return NULL;
}

int main() {
  pthread_t tid;
  int err;

  pthread_attr_t attr;
  err = pthread_attr_init(&attr);
  if (err) {
    printf("pthread_attr_init() failed: %s\n", strerror(err));
    return -1;
  }
  err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (err != 0) {
    printf("pthread_attr_setdetachstate() failed: %s\n", strerror(err));
    return -1;
  }

  printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

  while (1) {
    err = pthread_create(&tid, &attr, mythread, NULL);
    if (err) {
      printf("main: pthread_create() failed: %s\n", strerror(err));
      return -1;
    }
  }
  return 0;
}

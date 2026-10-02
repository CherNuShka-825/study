#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *args) {
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  int a = 1;
  while (1) {
    a++;
  }
}

int main() {
  pthread_t tid;
  int err;

  err = pthread_create(&tid, NULL, mythread, NULL);
  if (err) {
    printf("main: pthread_create() failed: %s\n", strerror(err));
    return -1;
  }

  sleep(1);
  pthread_cancel(tid);

  err = pthread_join(tid, NULL);
  if (err) {
    printf("main: pthread_join() failed: %s\n", strerror(err));
    return -1;
  }

  return 0;
}

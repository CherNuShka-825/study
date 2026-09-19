#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *arg) {
  sleep(1);
  printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(),
         gettid());

  return (void *)"hello world";
}

int main() {
  pthread_t tid;
  int err;
  void *resv;

  printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

  err = pthread_create(&tid, NULL, mythread, NULL);
  if (err) {
    printf("main: pthread_create() failed: %s\n", strerror(err));
    return -1;
  }

  err = pthread_join(tid, &resv);
  if (err) {
    printf("main: pthread_join() failed: %s\n", strerror(err));
    return -1;
  }

  printf("%s\n", (char *)resv);

  return 0;
}

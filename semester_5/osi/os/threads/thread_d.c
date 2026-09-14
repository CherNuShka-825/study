#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int global = 1;

void *mythread(void *arg) {
  printf("mythread [%d %d %d %lu]: Hello from mythread!\n", getpid(), getppid(),
         gettid(), pthread_self());
  int local = 1;
  local++;
  global++;
  printf("\tlocal:  %d\n", local);
  printf("\tglobal: %d\n", global);
  return NULL;
}

int main() {
  printf("main [%d %d %d %lu]: Hello from main!\n", getpid(), getppid(),
         gettid(), pthread_self());
  pthread_t tid[5];
  int err;
  for (int i = 0; i < 5; i++) {
    err = pthread_create(&tid[i], NULL, mythread, NULL);
    if (err) {
      printf("main: pthread_create() failed: %s\n", strerror(err));
      return -1;
    }
  }
  for (int i = 0; i < 5; i++) {
    pthread_join(tid[i], NULL);
  }
  return 0;
}

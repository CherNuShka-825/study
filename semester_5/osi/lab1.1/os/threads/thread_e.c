#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int global = 4;
const int global_const = 5;

void *mythread(void *arg) {
  printf("mythread [%d %d %d %lu]: Hello from mythread!\n", getpid(), getppid(),
         gettid(), (long unsigned int)pthread_self());
  int local = 1;
  static int static_local = 2;
  const int const_local = 3;
  printf("\tlocal:        %p\n", (void *)&local);
  printf("\tstatic local: %p\n", (void *)&static_local);
  printf("\tconst local:  %p\n", (void *)&const_local);
  printf("\tglobal:       %p\n", (void *)&global);
  printf("\tglobal const: %p\n", (void *)&global_const);
  sleep(15);
  return NULL;
}

int main() {
  printf("main [%d %d %d %lu]: Hello from main!\n", getpid(), getppid(),
         gettid(), (long unsigned int)pthread_self());
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

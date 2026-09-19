#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

struct Args {
  int number;
  char *text;
};

void *mythread(void *arg) {
  sleep(1);
  printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(),
         gettid());

  struct Args *args = arg;
  printf("%d\n", args->number);
  printf("%s\n", args->text);

  return NULL;
}

int main() {
  pthread_t tid;
  int err;
  struct Args args;
  args.number = 67;
  args.text = "hi";

  printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

  err = pthread_create(&tid, NULL, mythread, &args);
  if (err) {
    printf("main: pthread_create() failed: %s\n", strerror(err));
    return -1;
  }

  err = pthread_join(tid, NULL);
  if (err) {
    printf("main: pthread_join() failed: %s\n", strerror(err));
    return -1;
  }

  return 0;
}

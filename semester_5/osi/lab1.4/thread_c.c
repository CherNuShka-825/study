#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void cleanup(void *arg) {
  free(arg);
  printf("pochistil");
}

void *mythread(void *args) {
  char *str = malloc(strlen("privet mir\n") + 1);
  if (str == NULL) {
    return NULL;
  }

  strcpy(str, "privet mir\n");

  pthread_cleanup_push(cleanup, str);

  while (1) {
    printf("%s", str);
  }

  pthread_cleanup_pop(1);
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

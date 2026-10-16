#include <bits/types/sigset_t.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#define _GNU_SOURCE

void handler(int sig) {
  const char msg[] = "SIGINT recived\n";
  write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

void *mythread1(void *args) {
  int err;
  sigset_t set;

  if (sigemptyset(&set) == -1) {
    perror("sigemptyset");
    return NULL;
  }

  if (sigaddset(&set, SIGINT) == -1) {
    perror("sigaddset");
    return NULL;
  }

  err = pthread_sigmask(SIG_UNBLOCK, &set, NULL);
  if (err != 0) {
    fprintf(stderr, "pthread_sigmask: %s\n", strerror(err));
    return NULL;
  }

  while (1) {
    pause();
  }

  return NULL;
}

void *mythread2(void *args) {
  int err;
  sigset_t set;
  int sig;

  if (sigemptyset(&set) == -1) {
    perror("sigemptyset");
    return NULL;
  }

  if (sigaddset(&set, SIGQUIT) == -1) {
    perror("sigaddset");
    return NULL;
  }

  while (1) {
    err = sigwait(&set, &sig);
    if (err != 0) {
      fprintf(stderr, "sigwait: %s\n", strerror(err));
      return NULL;
    }

    printf("SIGQUIT recived\n");
  }
  return NULL;
}

int main() {
  printf("main %d: Hello from main!\n", getpid());
  int err;
  pthread_t tid1, tid2;

  sigset_t set;
  if (sigfillset(&set) == -1) {
    perror("sigfillset");
    return -1;
  }
  err = pthread_sigmask(SIG_BLOCK, &set, NULL);
  if (err != 0) {
    printf("pthread_sigmask: %s\n", strerror(err));
    return -1;
  }

  struct sigaction action = {0};
  action.sa_handler = handler;
  if (sigaction(SIGINT, &action, NULL) == -1) {
    perror("sigaction");
    return -1;
  }

  err = pthread_create(&tid1, NULL, mythread1, NULL);
  if (err) {
    printf("main: pthread_create() failed: %s\n", strerror(err));
    return -1;
  }

  err = pthread_create(&tid2, NULL, mythread2, NULL);
  if (err) {
    printf("main: pthread_create() failed: %s\n", strerror(err));
    return -1;
  }

  err = pthread_join(tid1, NULL);
  if (err) {
    printf("main: pthread_join() failed: %s\n", strerror(err));
    return -1;
  }
  err = pthread_join(tid2, NULL);
  if (err) {
    printf("main: pthread_join() failed: %s\n", strerror(err));
    return -1;
  }

  return 0;
}

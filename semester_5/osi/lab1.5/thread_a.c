#include <bits/types/sigset_t.h>
#include <signal.h>
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void handler(int sig) {
  const char msg[] = "SIGINT recived\n";
  write(STDOUT_FILENO, msg, sizeof(msg));
}

void *mythread1(void *args) {
  sigset_t set;

  sigemptyset(&set);
  sigaddset(&set, SIGINT);

  pthread_sigmask(SIG_UNBLOCK, &set, NULL);

  while (1) {
    pause();
  }

  return NULL;
}

void *mythread2(void *args) {
  sigset_t set;
  int sig;

  sigemptyset(&set);
  sigaddset(&set, SIGQUIT);

  while (1) {
    sigwait(&set, &sig);
    printf("SIGQUIT recived\n");
  }
  return NULL;
}

int main() {
  printf("main %d: Hello from main!\n", getpid());
  pthread_t tid1, tid2;
  int err;

  sigset_t set;
  sigfillset(&set);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  struct sigaction action = {0};
  action.sa_handler = handler;
  sigaction(SIGINT, &action, NULL);

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

  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);

  return 0;
}

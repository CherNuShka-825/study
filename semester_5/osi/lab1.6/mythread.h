#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#define MYTHREAD_CANCELED ((void *)-1)

typedef struct {
  struct mythread_state *state;
  unsigned long long generation;
} mythread_t;

int mythread_create(mythread_t *thread, void *(*start_routine)(void *),
                    void *arg);

int mythread_join(mythread_t *thread, void **result);

int mythread_detach(mythread_t *thread);

void mythread_exit(void *result);

int mythread_cancel(mythread_t *handle);

void mythread_testcancel(void);

#endif

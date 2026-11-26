#ifndef MYTHREAD_H
#define MYTHREAD_H

typedef struct mythread {
  struct mythread_state *state;
} mythread_t;

int mythread_create(mythread_t *thread, void *(*start_routine)(void *),
                    void *arg);

#endif

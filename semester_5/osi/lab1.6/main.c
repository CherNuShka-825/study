#include "mythread.h"

#include <stdatomic.h>

// Обычное завершение
static void *worker(void *arg) { return arg; }

// Явное завершение
static void *exit_worker(void *arg) {
  mythread_exit(arg);
  return NULL;
}

// Отложенная отмена
static void *cancel_worker(void *arg) {
  (void)arg;

  while (1) {
    mythread_testcancel();
  }

  return NULL;
}

// Detached-поток
static void *detach_worker(void *arg) {
  atomic_int *done = arg;
  atomic_store(done, 1);
  return NULL;
}

int main(void) {
  mythread_t thread;
  int number = 123;
  void *result = NULL;

  // 1. Создание и join
  if (mythread_create(&thread, worker, &number) == -1) {
    return 1;
  }

  if (mythread_join(&thread, &result) == -1) {
    return 2;
  }

  if (result != &number) {
    return 3;
  }

  // 2. Явный exit
  if (mythread_create(&thread, exit_worker, &number) == -1) {
    return 4;
  }

  if (mythread_join(&thread, &result) == -1) {
    return 5;
  }

  if (result != &number) {
    return 6;
  }

  // 3. Отложенная отмена
  if (mythread_create(&thread, cancel_worker, NULL) == -1) {
    return 7;
  }

  if (mythread_cancel(&thread) == -1) {
    return 8;
  }

  if (mythread_join(&thread, &result) == -1) {
    return 9;
  }

  if (result != MYTHREAD_CANCELED) {
    return 10;
  }

  // 4. Detached-поток
  atomic_int done = 0;

  if (mythread_create(&thread, detach_worker, &done) == -1) {
    return 11;
  }

  if (mythread_detach(&thread) == -1) {
    return 12;
  }

  while (!atomic_load(&done)) {
  }

  return 0;
}

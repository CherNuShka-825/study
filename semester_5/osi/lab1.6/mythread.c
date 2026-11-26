#define _GNU_SOURCE
#include "mythread.h"

#include <errno.h>
#include <linux/futex.h>
#include <sched.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#define MYTHREAD_STACK_SIZE (1024 * 1024)

typedef struct mythread_state {
  pid_t tid;
  void *stack;
  size_t size;
  void *(*start_routine)(void *);
  void *arg;
  struct mythread_state *next;
} mythread_state;

static mythread_state *thread_list_head;
static int thread_list_lock = 0;

static void lock_thread_list(void) {
  // функция компилятора gcc - заменяет по указанному и возвращает предыдущее
  // __ATOMIC_ACQUIRE - не позволяет последующим обращениям к памяти
  // переместится перед захватом
  while (__atomic_exchange_n(&thread_list_lock, 1, __ATOMIC_ACQUIRE) == 1) {
    long ret = syscall(SYS_futex, &thread_list_lock, FUTEX_WAIT_PRIVATE, 1,
                       NULL, NULL, 0);

    if (ret == -1 && errno != EAGAIN && errno != EINTR) {
      abort();
    }
  }
}

static void unlock_thread_list(void) {
  // записывает в память
  // __ATOMIC_ACQUIRE - не позволяет предыдущим обращениям к памяти
  // переместится псле захватa
  __atomic_store_n(&thread_list_lock, 0, __ATOMIC_RELEASE);

  if (syscall(SYS_futex, &thread_list_lock, FUTEX_WAKE_PRIVATE, 1, NULL, NULL,
              0) == -1) {
    abort();
  }
}

static mythread_state *allocate_state(void) {
  long page = sysconf(_SC_PAGESIZE);
  if (page <= 0) {
    return NULL;
  }

  mythread_state *state = calloc(1, sizeof(mythread_state));
  if (state == NULL) {
    return NULL;
  }

  size_t guard = (size_t)page;
  size_t usable = ((MYTHREAD_STACK_SIZE + page - 1) / page) * page;
  state->size = usable + guard;
  state->stack = mmap(NULL, state->size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
  if (state->stack == MAP_FAILED) {
    int saved = errno;
    free(state);
    errno = saved;
    return NULL;
  }

  if (mprotect(state->stack, guard, PROT_NONE) == -1) {
    int saved = errno;
    munmap(state->stack, state->size);
    free(state);
    errno = saved;
    return NULL;
  }
  return state;
}

static int thread_entry(void *arg) {
  mythread_state *state = arg;
  state->start_routine(state->arg);
  return 0;
}

int mythread_create(mythread_t *thread, void *(*start_routine)(void *),
                    void *arg) {
  if (thread == NULL || start_routine == NULL) {
    errno = EINVAL;
    return -1;
  }

  lock_thread_list();
  mythread_state *state = thread_list_head;
  while (state != NULL) {

    if (__atomic_load_n(&state->tid, __ATOMIC_RELAXED) == 0) {
      break;
    }
    state = state->next;
  }

  if (state == NULL) {
    state = allocate_state();
    if (state == NULL) {
      int saved = errno;
      unlock_thread_list();
      errno = saved;
      return -1;
    }
    state->next = thread_list_head;
    thread_list_head = state;
  }

  state->start_routine = start_routine;
  state->arg = arg;
  void *stack_top = (char *)state->stack + state->size;

  int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD |
              CLONE_SYSVSEM | CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID;

  int tid = clone(thread_entry, stack_top, flags, state, &state->tid, NULL,
                  &state->tid);
  if (tid == -1) {
    int saved = errno;
    unlock_thread_list();
    errno = saved;
    return -1;
  }

  thread->state = state;
  unlock_thread_list();
  return 0;
}

#define _GNU_SOURCE
#include "mythread.h"

#include <errno.h>
#include <linux/futex.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#define MYTHREAD_STACK_SIZE (1024 * 1024)

typedef struct mythread_state {
  pid_t tid;

  void *stack;
  size_t stack_size;

  void *(*start_routine)(void *);
  void *arg;

  _Atomic(void *) result;

  atomic_bool started;  // для регестрации в linced
  atomic_bool finished; // для join

  bool detached;
  bool joining;
  bool reserved;
  bool cancel_requested;

  struct mythread_state *next;

  unsigned long long generation;
} mythread_state;

static unsigned long long last_generation = 0;
static mythread_state *registry = NULL;
static atomic_flag registry_lock = ATOMIC_FLAG_INIT;

static void lock_registry(void) {
  while (atomic_flag_test_and_set(&registry_lock)) {
  }
}

static void unlock_registry(void) { atomic_flag_clear(&registry_lock); }

static void register_thread(mythread_state *thread) {
  lock_registry();

  thread->generation = ++last_generation;
  thread->next = registry;
  registry = thread;

  unlock_registry();
}

static void unregister_thread(mythread_state *thread) {
  lock_registry();

  mythread_state **p = &registry;

  while (*p != NULL && *p != thread) {
    p = &(*p)->next;
  }

  if (*p != NULL) {
    *p = thread->next;
  }

  unlock_registry();
}

static mythread_state *find_thread(mythread_t *handle) {
  for (mythread_state *p = registry; p != NULL; p = p->next) {
    if (p == handle->state && p->generation == handle->generation) {
      return p;
    }
  }

  return NULL;
}

static int thread_entry(void *arg) {
  mythread_state *thread = arg;

  while (!atomic_load(&thread->started)) {
  }

  mythread_testcancel();

  void *result = thread->start_routine(thread->arg);

  mythread_testcancel();

  mythread_exit(result);

  return 0;
}

int mythread_create(mythread_t *handle, void *(*start_routine)(void *),
                    void *arg) {
  if (handle == NULL || start_routine == NULL) {
    errno = EINVAL;
    return -1;
  }

  mythread_state *thread = NULL;
  bool reused = false;

  lock_registry();

  for (mythread_state *p = registry; p != NULL; p = p->next) {
    if (p->detached && !p->reserved && atomic_load(&p->finished) &&
        __atomic_load_n(&p->tid, __ATOMIC_ACQUIRE) == 0) {

      p->reserved = true;
      p->detached = false;
      p->joining = false;
      p->generation = ++last_generation;

      thread = p;
      reused = true;
      break;
    }
  }

  unlock_registry();

  if (!reused) {
    thread = malloc(sizeof(*thread));

    if (thread == NULL) {
      return -1;
    }

    long page_size = sysconf(_SC_PAGESIZE);

    if (page_size == -1) {
      free(thread);
      return -1;
    }

    size_t size = MYTHREAD_STACK_SIZE + (size_t)page_size;

    void *stack = mmap(NULL, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);

    if (stack == MAP_FAILED) {
      int error = errno;
      free(thread);
      errno = error;
      return -1;
    }

    if (mprotect(stack, (size_t)page_size, PROT_NONE) == -1) {
      int error = errno;

      if (munmap(stack, size) == -1) {
        free(thread);
        return -1;
      }

      free(thread);
      errno = error;
      return -1;
    }

    thread->stack = stack;
    thread->stack_size = size;
    thread->tid = 0;
    thread->detached = false;
    thread->joining = false;
    thread->next = NULL;
  }

  thread->start_routine = start_routine;
  thread->arg = arg;
  thread->cancel_requested = false;

  if (reused) {
    atomic_store(&thread->result, NULL);
    atomic_store(&thread->started, 0);
    atomic_store(&thread->finished, 0);
  } else {
    atomic_init(&thread->result, NULL);
    atomic_init(&thread->started, 0);
    atomic_init(&thread->finished, 0);

    register_thread(thread);
  }

  int flags =
      CLONE_VM |            // общее виртуальное пространство
      CLONE_FS |            // общие параметры файловой системы
      CLONE_FILES |         // общие файловые дискрипторы
      CLONE_SIGHAND |       // общии настройки обработчиков сигналов
      CLONE_THREAD |        // общие tgid
      CLONE_PARENT_SETTID | // ставит tid нового потока в память родителя
      CLONE_CHILD_CLEARTID; // при завершении потока обнуляет tid и разбудить
                            // ожидающего через futex

  pid_t tid = clone(thread_entry, (char *)thread->stack + thread->stack_size,
                    flags, thread, &thread->tid, NULL, &thread->tid);

  if (tid == -1) {
    if (reused) {
      int error = errno;

      lock_registry();

      atomic_store(&thread->finished, 1);
      thread->detached = true;
      thread->reserved = false;

      unlock_registry();

      errno = error;
      return -1;
    }

    int error = errno;

    unregister_thread(thread);

    if (munmap(thread->stack, thread->stack_size) == -1) {
      free(thread);
      return -1;
    }

    free(thread);
    errno = error;
    return -1;
  }

  handle->state = thread;
  handle->generation = thread->generation;

  lock_registry();
  thread->reserved = false;
  unlock_registry();

  atomic_store(&thread->started, 1);

  return 0;
}

int mythread_join(mythread_t *handle, void **result) {
  if (handle == NULL) {
    errno = EINVAL;
    return -1;
  }

  pid_t self = syscall(SYS_gettid);
  if (self == -1) {
    return -1;
  }

  lock_registry();

  mythread_state *thread = find_thread(handle);

  if (thread == NULL || thread->detached || thread->joining) {
    unlock_registry();
    errno = EINVAL;
    return -1;
  }

  if (__atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE) == self) {
    unlock_registry();
    errno = EDEADLK;
    return -1;
  }

  thread->joining = true;

  unlock_registry();

  while (1) {
    pid_t tid = __atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE);

    if (tid == 0) {
      break;
    }

    if (syscall(SYS_futex, &thread->tid, FUTEX_WAIT, tid, NULL, NULL, 0) ==
        -1) {
      if (errno != EAGAIN && errno != EINTR) { // tid не изменилось до усыпления
                                               // && не прерванно сигналом
        int error = errno;

        lock_registry();
        thread->joining = false;
        unlock_registry();

        errno = error;
        return -1;
      }
    }
  }

  if (!atomic_load(&thread->finished)) {
    lock_registry();
    thread->joining = false;
    unlock_registry();

    errno = EINVAL;
    return -1;
  }

  void *value = atomic_load(&thread->result);

  if (munmap(thread->stack, thread->stack_size) == -1) {
    int error = errno;

    lock_registry();
    thread->joining = false;
    unlock_registry();

    errno = error;
    return -1;
  }

  lock_registry();
  handle->state = NULL;
  unlock_registry();

  unregister_thread(thread);
  free(thread);

  if (result != NULL) {
    *result = value;
  }

  return 0;
}

int mythread_detach(mythread_t *handle) {
  if (handle == NULL) {
    errno = EINVAL;
    return -1;
  }

  lock_registry();

  mythread_state *thread = find_thread(handle);

  if (thread == NULL || thread->detached || thread->joining) {
    unlock_registry();
    errno = EINVAL;
    return -1;
  }

  thread->detached = true;

  unlock_registry();

  return 0;
}

void mythread_exit(void *result) {
  pid_t tid = syscall(SYS_gettid);

  if (tid == -1) {
    syscall(SYS_exit, 1); // ток поток
    __builtin_trap();     // вызывает SIGILL, весь процесс
  }
  lock_registry();

  mythread_state *thread = registry;

  while (thread != NULL) {
    if (__atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE) == tid) {
      break;
    }

    thread = thread->next;
  }

  if (thread != NULL) {
    atomic_store(&thread->result, result);
    atomic_store(&thread->finished, 1);
  }

  unlock_registry();

  syscall(SYS_exit, thread == NULL ? 1 : 0);
  __builtin_trap();
}

int mythread_cancel(mythread_t *handle) {
  if (handle == NULL) {
    errno = EINVAL;
    return -1;
  }

  lock_registry();

  mythread_state *thread = find_thread(handle);

  if (thread == NULL || atomic_load(&thread->finished) ||
      __atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE) == 0) {
    unlock_registry();
    errno = ESRCH; // no such process
    return -1;
  }

  thread->cancel_requested = true;

  unlock_registry();

  return 0;
}

void mythread_testcancel(void) {
  pid_t tid = syscall(SYS_gettid);

  if (tid == -1) {
    return;
  }

  bool cancel = false;

  lock_registry();

  mythread_state *thread = registry;

  while (thread != NULL) {
    if (__atomic_load_n(&thread->tid, __ATOMIC_ACQUIRE) == tid) {
      cancel = thread->cancel_requested;
      break;
    }

    thread = thread->next;
  }

  unlock_registry();

  if (cancel) {
    mythread_exit(MYTHREAD_CANCELED);
  }
}

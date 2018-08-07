// Basic userspace handshake using futexes, for two processes.
//
// Eli Bendersky [http://eli.thegreenplace.net]
// This code is in the public domain.
#define _GNU_SOURCE // for sched_setaffinity
#include <errno.h>
#include <linux/futex.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Using 64-bit values saves one instruction clearing the high half of low */
#define DECLARE_ARGS(val, low, high)    unsigned long low, high
#define EAX_EDX_VAL(val, low, high)     ((low) | (high) << 32)
#define EAX_EDX_RET(val, low, high)     "=a" (low), "=d" (high)

static __always_inline unsigned long long rdtsc(void)
{
        DECLARE_ARGS(val, low, high);

        asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));

        return EAX_EDX_VAL(val, low, high);
}

// The C runtime doesn't provide a wrapper for the futex(2) syscall, so we roll
// our own.
int futex(int* uaddr, int futex_op, int val, const struct timespec* timeout,
          int* uaddr2, int val3) {
  return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

// Waits for the futex at futex_addr to have the value val, ignoring spurious
// wakeups. This function only returns when the condition is fulfilled; the only
// other way out is aborting with an error.
void wait_on_futex_value(int* futex_addr, int val) {
  while (1) {
    int futex_rc = futex(futex_addr, FUTEX_WAIT, val, NULL, NULL, 0);
    if (futex_rc == -1) {
      if (errno != EAGAIN) {
        perror("futex");
        exit(1);
      }
    } else if (futex_rc == 0) {
      if (*futex_addr == val) {
        // This is a real wakeup.
        return;
      }
    } else {
      abort();
    }
  }
}

// A blocking wrapper for waking a futex. Only returns when a waiter has been
// woken up.
void wake_futex_blocking(int* futex_addr) {
  while (1) {
    int futex_rc = futex(futex_addr, FUTEX_WAKE, 1, NULL, NULL, 0);
    if (futex_rc == -1) {
      perror("futex wake");
      exit(1);
    } else if (futex_rc > 0) {
      return;
    }
  }
}

static inline int set_sched_affinity(int affinity)
{
    pid_t pid = getpid();
    cpu_set_t my_set;
    int ret;

    CPU_ZERO(&my_set);
    CPU_SET(affinity, &my_set);

    ret = sched_setaffinity(pid, sizeof(my_set), &my_set);
    if (ret < 0) {
        perror("[UIPC] set affinity failed...\n");
        return -1;
    }

    return 0;
}

static uint64_t before_wakeup, after_wakeup;
#define PROCESSA_WAIT 0xC
#define PROCESSB_WAIT 0xD

int main(int argc, char** argv) {
  int shm_id = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0666);
  if (shm_id < 0) {
    perror("shmget");
    exit(1);
  }
  int* shared_data = shmat(shm_id, NULL, 0);
  *shared_data = 0;

  int forkstatus = fork();
  if (forkstatus < 0) {
    perror("fork");
    exit(1);
  }

  if (forkstatus == 0) {
    // Child process
    if (set_sched_affinity(5) < 0) {
        perror("set_sched_affinity failed in child");
        return 0;
    }

    printf("child waiting for A\n");
    for (int i = 0; i < 10000; ++i) {
        wait_on_futex_value(shared_data, PROCESSA_WAIT);
        after_wakeup = rdtsc();
        printf("after time = 0x%lx\n", after_wakeup);
    }

    printf("child writing B\n");
    // Write 0xB to the shared data and wake up parent.
    *shared_data = PROCESSB_WAIT;
    wake_futex_blocking(shared_data);
  } else {
    // Parent process.
    if (set_sched_affinity(7) < 0) {
        perror("set_sched_affinity failed in child");
        return 0;
    }

    printf("parent writing A\n");
    for (int i = 0; i < 10000; ++i) {
        before_wakeup = rdtsc();
        // Write 0xA to the shared data and wake up child.
        *shared_data = PROCESSA_WAIT;
        wake_futex_blocking(shared_data);
        printf("before time = 0x%lx\n", before_wakeup);
    }

    printf("parent waiting for B\n");
    wait_on_futex_value(shared_data, PROCESSB_WAIT);

    // Wait for the child to terminate.
    wait(NULL);
    shmdt(shared_data);
  }

  return 0;
}


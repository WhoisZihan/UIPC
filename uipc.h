#ifndef UIPC_H_
#define UIPC_H_

#define UIPC_ENTER_MONITOR_MWAIT 0x1313
#define UIPC_TRIGGER_MONITOR     0x1323

#define MONITOR_MWAIT_FLAG (1 << 3)

static inline void uipc_native_cpuid(unsigned int *eax, unsigned int *ebx,
				unsigned int *ecx, unsigned int *edx)
{
	/* ecx is often an input as well as an output. */
	asm volatile("cpuid"
	    : "=a" (*eax),
	      "=b" (*ebx),
	      "=c" (*ecx),
	      "=d" (*edx)
	    : "0" (*eax), "2" (*ecx)
	    : "memory");
}

#define __uipc_cpuid uipc_native_cpuid

/*
 * Generic CPUID function borrowed from Linux kernel
 * clear %ecx since some cpus (Cyrix MII) do not set or clear %ecx
 * resulting in stale register contents being returned.
 */
static inline void uipc_cpuid(unsigned int op,
			 unsigned int *eax, unsigned int *ebx,
			 unsigned int *ecx, unsigned int *edx)
{
	*eax = op;
	*ecx = 0;
	__uipc_cpuid(eax, ebx, ecx, edx);
}
#endif

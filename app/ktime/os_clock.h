#ifndef __OS_CLOCK_H__
#define __OS_CLOCK_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>

static inline double
unix_time_now(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + 1e-9 * ts.tv_nsec;
}

static inline uint64_t
unix_time_now_nsec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    return 1e9 * ts.tv_sec + ts.tv_nsec;
}

#if defined(__x86_64__) || defined(i386)
static inline uint64_t
cpu_tsc_now(void)
{
    uint32_t a, d;

    asm volatile("rdtsc":"=a"(a), "=d"(d));
    return (uint64_t) a + ((uint64_t) d << (uint64_t) 32);
}
#elif defined (__aarch64__)
static inline uint64_t
cpu_tsc_now(void)
{
    uint64_t vct;

    /* User access to cntvct_el0 is enabled in Linux kernel since 3.12. */
    asm volatile("mrs %0, cntvct_el0":"=r"(vct));
    return vct;
}
#else
#error "### unsupport"
#endif

static inline double
estimate_clock_frequency(double sample_time)
{
    double time_now, time_start, time_limit;
    uint64_t t[2];

    time_start = time_now = unix_time_now();
    time_limit = time_now + sample_time;
    t[0] = cpu_tsc_now();
    do {
        sched_yield();
        time_now = unix_time_now();
    } while (time_now < time_limit);
    t[1] = cpu_tsc_now();

    return (t[1] - t[0]) / (time_now - time_start);
}

static inline uint64_t
cpu_freq()
{
    static uint64_t __cpu_freq;

    if (!__cpu_freq) {
#if defined (__aarch64__)
        /* The system counter increments at a fixed frequency. It is distributed
         * to each core which has registers for reading the current counter value
         * as well as the clock frequency. The system counter is not clocked at
         * the same frequency as the core. */
        asm volatile("mrs %0, cntfrq_el0":"=r"(__cpu_freq));
#else
        __cpu_freq = (uint64_t)estimate_clock_frequency(1e-2);
#endif
    }

    return __cpu_freq;
}

#endif // __OS_CLOCK_H__
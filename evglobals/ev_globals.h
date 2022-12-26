#ifndef EV_GLOBALS_H_INCLUDED
#define EV_GLOBALS_H_INCLUDED

#include  <stdint.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <time.h>


__BEGIN_DECLS

/*

### void ev_init_globals()

#include <ev_globals.h>

DESCRIPTION:
Initializes the global variables needed for evio (file library) to work.

 */
void ev_init_globals();


/*

### size_t get_sys_pagesize()

#include <ev_globals.h>

DESCRIPTION:
Returns the size of a memory page as set in sysconf

It is necessary that ev_init_globals is called before this

 */
size_t get_sys_pagesize();

#ifndef __APPLE__
/*

### uint64_t clock_gettime_nsec_np()

#include <ev_globals.h>

DESCRIPTION:
Implementation of clock_gettime_nsec_np for linux and other non OSX systems

INPUT:
clockid_t clock_id

CLOCK_REALTIME     the system's real time (i.e. wall time) clock, expressed as the amount of time since the Epoch.  This is the
                   same as the value returned by gettimeofday(2).

CLOCK_MONOTONIC    clock that increments monotonically, tracking the time since an arbitrary point, and will continue to increment
                   while the system is asleep.

CLOCK_MONOTONIC_RAW
                   clock that increments monotonically, tracking the time since an arbitrary point like CLOCK_MONOTONIC.  However,
                   this clock is unaffected by frequency or time adjustments.  It should not be compared to other system time
                   sources.

CLOCK_MONOTONIC_RAW_APPROX
                   like CLOCK_MONOTONIC_RAW, but reads a value cached by the system at context switch.  This can be read faster,
                   but at a loss of accuracy as it may return values that are milliseconds old.

CLOCK_UPTIME_RAW   clock that increments monotonically, in the same manner as CLOCK_MONOTONIC_RAW, but that does not increment
                   while the system is asleep.  The returned value is identical to the result of mach_absolute_time() after the
                   appropriate mach_timebase conversion is applied.

CLOCK_UPTIME_RAW_APPROX
                   like CLOCK_UPTIME_RAW, but reads a value cached by the system at context switch.  This can be read faster, but
                   at a loss of accuracy as it may return values that are milliseconds old.

CLOCK_PROCESS_CPUTIME_ID
                   clock that tracks the amount of CPU (in user- or kernel-mode) used by the calling process.

CLOCK_THREAD_CPUTIME_ID
                   clock that tracks the amount of CPU (in user- or kernel-mode) used by the calling thread.


RETURN:
Non zero value indicates success and the clock time in nanoseconds
When zero is returned appropriate error is set in errno


 */
uint64_t clock_gettime_nsec_np(clockid_t clock_id);

#endif
__END_DECLS


#endif

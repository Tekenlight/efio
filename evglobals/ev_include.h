#ifndef EV_INCLUDE_H_INCLUDED
#define EV_INCLUDE_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
/*
int noprintf(char * s, ...)
{
	return 0;
}
*/

#define EV_ABORT(...) { \
	printf("[%p]:[%s:%d] ABORTING:",pthread_self(),__FILE__,__LINE__);fflush(stdout); \
	printf(__VA_ARGS__); printf("\n");fflush(stdout); \
	perror("SYSERR:"); \
	abort(); \
}


#ifdef EV_DEBUG

#define EV_DBG() { \
	printf("[%p][%s:%d] Reached\n",pthread_self(),__FILE__,__LINE__);fflush(stdout); \
}

#define EV_DBGP(...) { \
	printf("[%p][%s:%d] Reached:",pthread_self(),__FILE__,__LINE__); \
	printf(__VA_ARGS__);fflush(stdout); \
}

#else

#define EV_DBG() {}
#define EV_DBGP(...) {}

#endif
#ifdef __APPLE__

#include "TargetConditionals.h"

#if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR

#if defined TARGET_OS_ONLY_IPHONE_SIMULATOR
#undef TARGET_OS_ONLY_IPHONE_SIMULATOR
#endif
#define TARGET_OS_ONLY_IPHONE_SIMULATOR 1

#elif TARGET_OS_IPHONE

#if defined TARGET_OS_UNION_IPHONE
#undef TARGET_OS_UNION_IPHONE
#endif
#define TARGET_OS_UNION_IPHONE 1

#else

#if defined TARGET_OS_OSX
#undef TARGET_OS_OSX
#endif
#define TARGET_OS_OSX 1

#endif

#endif
#endif

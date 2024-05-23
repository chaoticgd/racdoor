#ifndef _RACDOOR_UTIL_H
#define _RACDOOR_UTIL_H

#ifndef _EE
	#include <stdio.h>
	#include <stdlib.h>
#endif
	
#ifdef _EE
	#define CHECK() if(!(condition)) __builtin_trap();
#else
	#define CHECK(condition, ...) if(!(condition)) { fprintf(stderr, __VA_ARGS__); exit(1); }
#endif

static inline int32_t align32(int32_t value, int32_t alignment)
{
	return value + (-value & (alignment - 1));
}

#ifndef _EE
	static inline void* checked_malloc(unsigned long size)
	{
		void* ptr = malloc(size);
		CHECK(ptr || size == 0, "Failed to allocate memory.\n");
		return ptr;
	}
#endif

#endif

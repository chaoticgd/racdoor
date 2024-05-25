#ifndef _RACDOOR_UTIL_H
#define _RACDOOR_UTIL_H

#ifndef _EE
	#include <stdio.h>
	#include <stdlib.h>
#endif

#define MIN(x, y) ((x < y) ? (x) : (y))
#define MAX(x, y) ((x > y) ? (x) : (y))

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#ifdef _EE
	#define ERROR(...) __builtin_trap();
#else
	#define ERROR(...) do { fprintf(stderr, "ERROR: " __VA_ARGS__); exit(1); } while(0);
#endif

#define CHECK(condition, ...) if(!(condition)) { ERROR(__VA_ARGS__); }

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

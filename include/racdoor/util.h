#ifndef _RACDOOR_UTIL_H
#define _RACDOOR_UTIL_H

#ifndef _EE
	#include <stdio.h>
	#include <stdlib.h>
#endif

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;

#define MIN(x, y) ((x < y) ? (x) : (y))
#define MAX(x, y) ((x > y) ? (x) : (y))

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#ifdef _EE
	#define ERROR(...) __builtin_trap();
#else
	#define ERROR(...) do { fprintf(stderr, "ERROR: " __VA_ARGS__); exit(1); } while(0);
#endif

#define CHECK(condition, ...) if(!(condition)) { ERROR(__VA_ARGS__); }

#ifndef _EE
	static inline int32_t align32(int32_t value, int32_t alignment)
	{
		return value + (-value & (alignment - 1));
	}
	
	static inline void* checked_malloc(unsigned long size)
	{
		void* ptr = malloc(size);
		CHECK(ptr || size == 0, "Failed to allocate memory.\n");
		return ptr;
	}
#endif

#endif

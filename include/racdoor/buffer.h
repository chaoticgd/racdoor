/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#ifndef _RACDOOR_BUFFER_H
#define _RACDOOR_BUFFER_H

#ifdef _HOST

#include <racdoor/util.h>

/* A buffer of data. */
typedef struct {
	char* data;
	u32 size;
} Buffer;

/* Read a file from the filesystem. */
Buffer read_file(const char* path);

/* Write a file to the filesystem. */
void write_file(const char* path, Buffer buffer);

/* Get a pointer to an object in the buffer. */
void* buffer_get(Buffer buffer, u32 offset, u32 size, const char* thing);

/* Get a pointer to a null-terminated string in the buffer. */
const char* buffer_string(Buffer buffer, u32 offset, const char* thing);

/* Build a new buffer object pointing into the passed buffer. */
Buffer sub_buffer(Buffer buffer, u32 offset, u32 size, const char* thing);

#endif

#endif

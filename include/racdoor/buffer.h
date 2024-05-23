#ifndef _RACDOOR_BUFFER_H
#define _RACDOOR_BUFFER_H

#include <racdoor/util.h>

#include <stdint.h>
#include <stdlib.h>

typedef struct {
	char* data;
	int32_t size;
} Buffer;

static inline Buffer read_file(const char* path)
{
	FILE* file = fopen(path, "rb");
	CHECK(file, "Failed to open input file '%s'.\n", path);
	
	CHECK(fseek(file, 0, SEEK_END) == 0, "Failed to seek to beginning of input file '%s'.\n", path);
	long file_size = ftell(file);
	CHECK(file_size > 0 && file_size < UINT32_MAX, "Cannot determine file size for input file '%s'.\n", path);
	
	char* file_data = checked_malloc(file_size);
	CHECK(fseek(file, 0, SEEK_SET) == 0, "Failed to seek to beginning of input file '%s'.\n", path);
	CHECK(fread(file_data, file_size, 1, file) == 1, "Failed to read input file '%s'.\n", path);
	
	fclose(file);
	
	Buffer buffer = {
		.data = file_data,
		.size = file_size
	};
	return buffer;
}

static inline void write_file(const char* path, Buffer buffer)
{
	FILE* file = fopen(path, "wb");
	CHECK(file, "Failed to open output file '%s'.\n", path);
	
	CHECK(fwrite(buffer.data, buffer.size, 1, file) == 1, "Failed to write output file '%s'.\n", path);
	
	fclose(file);
}

static inline void* buffer_get(Buffer buffer, int32_t offset, int32_t size, const char* thing)
{
	CHECK(offset > -1 && offset + size <= buffer.size, "Out of bounds %s.\n", thing);
	return buffer.data + offset;
}

static inline Buffer sub_buffer(Buffer buffer, int32_t offset, int32_t size, const char* thing)
{
	CHECK(offset > -1 && offset + size <= buffer.size, "Out of bounds %s.\n", thing);
	Buffer result = {
		.data = buffer.data + offset,
		.size = size
	};
	return result;
}

#endif

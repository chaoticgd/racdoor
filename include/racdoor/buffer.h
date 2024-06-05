#ifndef _RACDOOR_BUFFER_H
#define _RACDOOR_BUFFER_H

#include <racdoor/util.h>

#include <stdlib.h>

typedef struct {
	char* data;
	u32 size;
} Buffer;

static inline Buffer read_file(const char* path)
{
	FILE* file = fopen(path, "rb");
	CHECK(file, "Failed to open input file '%s'.\n", path);
	
	CHECK(fseek(file, 0, SEEK_END) == 0, "Failed to seek to beginning of input file '%s'.\n", path);
	long file_size = ftell(file);
	CHECK(file_size >= 0 && file_size <= 0xffffffff, "Cannot determine file size for input file '%s'.\n", path);
	
	char* file_data = checked_malloc(file_size + 1);
	if (file_size > 0)
	{
		CHECK(fseek(file, 0, SEEK_SET) == 0, "Failed to seek to beginning of input file '%s'.\n", path);
		CHECK(fread(file_data, file_size, 1, file) == 1, "Failed to read input file '%s'.\n", path);
	}
	file_data[file_size] = 0;
	
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
	
	if (buffer.size > 0)
		CHECK(fwrite(buffer.data, buffer.size, 1, file) == 1, "Failed to write output file '%s'.\n", path);
	
	fclose(file);
}

static inline void* buffer_get(Buffer buffer, u32 offset, u32 size, const char* thing)
{
	CHECK((u64) offset + size <= buffer.size, "Out of bounds %s.\n", thing);
	return buffer.data + offset;
}

static inline const char* buffer_string(Buffer buffer, u32 offset, const char* thing)
{
	for (char* ptr = buffer.data + offset; ptr < buffer.data + buffer.size; ptr++)
		if (*ptr == '\0')
			return (const char*) &buffer.data[offset];
	ERROR("Out of bounds %s.\n", thing);
}

static inline Buffer sub_buffer(Buffer buffer, u32 offset, u32 size, const char* thing)
{
	CHECK((u64) offset + size <= buffer.size, "Out of bounds %s.\n", thing);
	Buffer result = {
		.data = buffer.data + offset,
		.size = size
	};
	return result;
}

#endif

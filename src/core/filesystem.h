#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdbool.h>
#include <stddef.h>

bool file_exists(const char* filepath);

bool file_read_all(const char* filepath, char** data, size_t* size);

#endif // FILESYSTEM_H_

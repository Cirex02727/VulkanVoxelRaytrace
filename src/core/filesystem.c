#include "filesystem.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

bool file_exists(const char* filepath)
{
    return access(filepath, F_OK) == 0;
}

bool file_read_all(const char* filepath, char** data, size_t* size)
{
    FILE* f = fopen(filepath, "rb");
    if(f == NULL)
    {
        printf("[ERROR] File read all open: %s\n", filepath);
        return false;
    }

    fseek(f, 0, SEEK_END);
    *size = (size_t) ftell(f);
    fseek(f, 0, SEEK_SET);

    *data = (char*)malloc(*size + 1);
    fread(*data, *size, 1, f);
    fclose(f);
    
    (*data)[*size] = '\0';
    return true;
}

#ifndef BUILDER_H_
#define BUILDER_H_

#include <sys/wait.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <utime.h>
#include <time.h>

// ##### Dynamic Array #####

#define LIST_DEFAULT_CAPACITY 10

void list_destroy_(void** items)
{
    assert(items != NULL);

    free(*items);
    *items = NULL;
}

void list_alloc_(void** items, size_t* count, size_t* capacity, size_t typeSize, size_t startCapacity)
{
    if(*items != NULL)
    {
        printf("[Warning] List realloc\n");
        list_destroy_(items);
    }

    *(char**)items = (char*)malloc(startCapacity * typeSize);
    *count = 0;
    *capacity = startCapacity;
}

void list_grow_(void** items, size_t* capacity, size_t typeSize)
{
    size_t prevCapacity = *capacity;
    *capacity *= 2;

    void* prevItems = *items;
    *(char**)items = (char*)malloc(*capacity * typeSize);
    memcpy(*items, prevItems, prevCapacity * typeSize);

    free(prevItems);
}

void* list_append_(void** items, size_t* count, size_t* capacity, void* item, size_t itemCount, size_t typeSize)
{
    if(*items == NULL)
        list_alloc_(items, count, capacity, typeSize, LIST_DEFAULT_CAPACITY);
    
    size_t itemsStride = itemCount * typeSize;
    if(*count + itemCount >= *capacity)
        list_grow_(items, capacity, itemsStride);
    
    void* ptr = *items + *count * typeSize;
    memcpy(ptr, item, itemsStride);
    *count += itemCount;

    return ptr;
}

#define list_alloc(list, startCapacity) list_alloc_((void**)&(list).items, &(list).count, &(list).capacity, sizeof(*((list).items)), startCapacity)

#define list_alloc_default(list) list_alloc(list, LIST_DEFAULT_CAPACITY)

#define list_alloc_sized(list, typeSize, startCapacity) list_alloc_((void**)&(list).items, &(list).count, &(list).capacity, typeSize, startCapacity)

#define list_alloc_sized_default(list, typeSize) list_alloc_sized(list, typeSize, LIST_DEFAULT_CAPACITY)


#define list_destroy(list) list_destroy_((void**)&(list).items);


#define list_append(list, item) list_append_((void**)&(list).items, &(list).count, &(list).capacity, &item, 1, sizeof(*((list).items)))

#define list_append_multiple(list, item, itemCount) list_append_((void**)&(list).items, &(list).count, &(list).capacity, &item, itemCount, sizeof(*((list).items)))

#define list_append_sized(list, item, typeSize) list_append_((void**)&(list).items, &(list).count, &(list).capacity, item, 1, typeSize)

#define list_append_sized_multiple(list, item, itemCount, typeSize) list_append_((void**)&(list).items, &(list).count, &(list).capacity, item, itemCount, typeSize)

// ##### Dynamic Array #####

// ##### STRING #####

typedef struct {
    char*  items;
    size_t count;
} String;

void string_destroy_(char** items)
{
    free(*items);
    *items = NULL;
}

#define string_destroy(str) string_destroy_(&(str).items)

// ##### STRING #####

// ##### STRING BUILDER #####

/*
 *  USAGE:
 *  
 *  StringBuilder sb;
 *  string_builder_alloc(sb);
 *  
 *  char* text = "Hello";
 *  string_builder_append(sb, text);
 *  
 *  text = " World";
 *  string_builder_append(sb, text);
 *  
 *  printf("SB: %s\n", string_builder_get(sb));
 */

typedef struct
{
    char*  items;
    size_t count;
    size_t capacity;
} StringBuilder;

#define STRING_BUILDER_DEFAULT_SIZE 128

#define string_builder_alloc(list) list_alloc(list, STRING_BUILDER_DEFAULT_SIZE)

#define string_builder_append(list, text) list_append_sized_multiple(list, text, strlen(text), sizeof(char))

#define string_builder_append_sized(list, text, itemSize) list_append_sized_multiple(list, text, itemSize, sizeof(char))

#define string_builder_destroy(list) list_destroy(list)

char* string_builder_get(StringBuilder* sb)
{
    const char* nullEnd = '\0';
    list_append_sized(*sb, &nullEnd, sizeof(char));

    return sb->items;
}

// ##### STRING BUILDER #####

// ##### STRING VIEW #####

/*
 *  USAGE:
 *  
 *  StringView sv = {
 *      .items = sb.items,
 *      .count = 8
 *  };
 *  
 *  printf("SV: "string_view_format, string_view_args(sv));
 */

typedef struct
{
    const char* items;
    size_t      count;
} StringView;

#define string_view_format "%.*s"
#define string_view_args(sv) (int)(sv).count, (sv).items

String* string_view_copy(StringView sv)
{
    String* newStr = (String*)malloc(sizeof(String));
    newStr->items = (char*)malloc(sv.count * sizeof(char));
    memcpy(newStr->items, sv.items, sv.count * sizeof(char));
    newStr->count = sv.count;
    return newStr;
}

// ##### STRING VIEW #####

// ##### File #####

char* builder_filename_no_extension(const char* filepath)
{
    int64_t len = strlen(filepath);
    int64_t dot = len - 1;
    while(dot >= 0 && filepath[dot] != '.') dot--;

    if(dot < 0 || dot == len - 1)
        return NULL;

    int64_t slash = dot - 1;
    while(slash >= 0 && filepath[slash] != '\\' && filepath[slash] != '/') slash--;

    if(slash < 0)
        return NULL;
    
    int64_t filenameLen = dot - slash - 1;
    if(filenameLen <= 0)
        return NULL;
    
    char* filename = (char*) malloc(filenameLen + 1);
    memcpy(filename, filepath + slash + 1, filenameLen);
    filename[filenameLen] = '\0';
    return filename;
}

char* builder_file_extenstion(const char* filepath)
{
    int64_t len = strlen(filepath);
    int64_t dot = len - 1;
    while(dot >= 0 && filepath[dot] != '.') dot--;

    if(dot < 0 || dot == len - 1)
        return NULL;
    
    char* extension = (char*) malloc(len - dot);
    strcpy(extension, filepath + dot + 1);
    return extension;
}

bool builder_valid_compiler_extension(const char* extension)
{
    if(strcmp(extension, "c") == 0) return true;
    return false;
}

bool file_exists(const char* filepath)
{
    return access(filepath, F_OK) == 0;
}

bool file_read_all(const char* filepath, String* s)
{
    FILE* f = fopen(filepath, "rb");
    if(f == NULL)
    {
        printf("[ERROR] File read all open: %s\n", filepath);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* str = (char*)malloc(size + 1);
    fread(str, size, 1, f);
    fclose(f);

    str[size] = 0;

    s->items = str;
    s->count = size;
    return true;
}

struct stat file_get_stats(const char* filepath)
{
    struct stat info;
    if(stat(filepath, &info) < 0)
    {
        printf("[ERROR] file get last modified time: %s\n", strerror(errno));
        return (struct stat) {0};
    }

    return info;
}

time_t file_get_last_modified_time(const char* filepath)
{
    return file_get_stats(filepath).st_mtime;
}

// ##### File #####

// ##### Directory #####

typedef struct {
    const char** items;
    size_t       count;
    size_t       capacity;
} Files;

void builder_get_files_in_directory_recursivelly(const char* dirpath, Files* files)
{
    DIR* dir = opendir (dirpath);
    if(!dir)
        return;

    struct dirent* dp;
    while ((dp = readdir(dir)) != NULL)
    {
        if(strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue;
        
        char* fullpath = (char*) malloc(strlen(dirpath) + strlen(dp->d_name) + 2);
        strcpy(fullpath, dirpath);
        strcat(fullpath, "/");
        strcat(fullpath, dp->d_name);

        if(dp->d_type == DT_DIR)
            builder_get_files_in_directory_recursivelly(fullpath, files);
        else
        {
            char* extension = builder_file_extenstion(fullpath);
            if(builder_valid_compiler_extension(extension))
                list_append(*files, fullpath);
            
            free(extension);
        }
    }

    closedir(dir);
}

// ##### Directory #####

// ##### Builder #####

typedef struct {
    const char** items;
    size_t       count;
    size_t       capacity;
} Bld_Cmd;

static bool run = false;

static int    runArgc = 0;
static char** runArgv = NULL;

static bool verbose = true;

void bld_set_verbose(bool v)
{
    verbose = v;
}

const char* bld_shift_argument(int* argc, char*** argv)
{
    if(*argc <= 0)
        return NULL;
    
    const char* arg = *argv[0];
    --(*argc);
    ++(*argv);
    return arg;
}

void bld_command_append_(Bld_Cmd* cmd, ...)
{
    va_list ap;
    va_start(ap, cmd);
    for(;;)
    {
        const char* c = va_arg(ap, const char*);
        if(c == NULL)
            break;
        
        list_append(*cmd, c);
    }
    va_end(ap);
}

#define bld_command_append(cmd, ...) bld_command_append_(cmd, __VA_ARGS__, NULL);

bool bld_command_exec(Bld_Cmd cmd, const char* command)
{
    int pid = fork();
    if(pid == -1)
    {
        printf("[ERROR] %s error forking\n", command);
        return 0;
    }
    
    // Father
    if(pid != 0)
    {
        int status = 0;
        waitpid(pid, &status, 0);

        if(WIFEXITED(status))
        {
            int code = WEXITSTATUS(status);
            if(code != EXIT_SUCCESS)
            {
                printf("[ERROR] %s exited with code: %d\n", command, code);
                return false;
            }
        }
        else
        {
            printf("[ERROR] %s exited abnormaly: %d\n", command, status);
            return false;
        }

        if(verbose)
            printf("[EXEC] %s successfully\n", command);
        return true;
    }

    Bld_Cmd cmdNull = {0};
    
    if(verbose)
    {
        printf("[EXEC] ");
        for(size_t i = 0; i < cmd.count; ++i)
        {
            printf("%s ", cmd.items[i]);
            list_append(cmdNull, cmd.items[i]);
        }
        printf("\n");
    }
    else
    {
        for(size_t i = 0; i < cmd.count; ++i)
            list_append(cmdNull, cmd.items[i]);
    }
    
    const char* n = NULL;
    list_append(cmdNull, n);
    
    if(execvp(cmd.items[0], (char* const*)cmdNull.items) < 0)
    {
        printf("[ERROR] %s exec command: %s\n", command, strerror(errno));
        exit(-1);
    }

    assert(0 && "[ERROR] invalid position");
    return false;
}

bool bld_command_build_and_reset(Bld_Cmd* cmd)
{
    bool res = bld_command_exec(*cmd, "build");
    cmd->count = 0;
    return res;
}

bool bld_command_run_and_reset(Bld_Cmd* cmd)
{
    if(!run)
        return true;
    
    for(int i = 0; i < runArgc; ++i)
        list_append(*cmd, runArgv[i]);
    
    bool res = bld_command_exec(*cmd, "run");
    cmd->count = 0;
    return res;
}

void bld_rebuild_yourself(int* argc, char*** argv)
{
    const char* program = bld_shift_argument(argc, argv);

    if(*argc > 0 && strcmp(*argv[0], "run") == 0)
    {
        bld_shift_argument(argc, argv);
        run = true;
    }

    runArgc = *argc;
    runArgv = *argv;

    char cprogram[4 * 1024];
    sprintf(cprogram, "%s.c", program);

    struct stat cprogramStats = file_get_stats(cprogram);

    // Rebuild
    if(cprogramStats.st_mtime != file_get_last_modified_time(program))
    {
        if(verbose)
            printf("[INFO] re-building\n");

        Bld_Cmd cmd = {0};
        bld_command_append(&cmd, "cc", "-ggdb", "-o", program, cprogram);
        if(!bld_command_exec(cmd, "rebuild"))
            return;
        cmd.count = 0;

        struct utimbuf time;
        time.actime = cprogramStats.st_atime;
        time.modtime = cprogramStats.st_mtime;
        utime(program, &time);

        list_append(cmd, program);
        if(run)
        {
            const char* r = "run";
            list_append(cmd, r);
        }

        for(int i = 0; i < runArgc; ++i)
            list_append(cmd, runArgv[i]);

        const char* n = NULL;
        list_append(cmd, n);
        
        if(execvp(cmd.items[0], (char* const*)cmd.items) < 0)
        {
            printf("[EXEC] rebuild exec command: %s\n", strerror(errno));
            exit(-1);
        }
    }
}

// ##### Builder #####

#endif // BUILDER_H_

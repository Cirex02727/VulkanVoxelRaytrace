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
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <utime.h>
#include <time.h>

// ##### STB Hash Map #####

typedef struct
{
  size_t      length;
  size_t      capacity;
  void      * hash_table;
  ptrdiff_t   temp;
} stbds_array_header;

// this macro takes the address of the argument, but on gcc/clang can accept rvalues
#if defined(STBDS_HAS_LITERAL_ARRAY) && defined(STBDS_HAS_TYPEOF)
  #if __clang__
  #define STBDS_ADDRESSOF(typevar, value)     ((__typeof__(typevar)[1]){value}) // literal array decays to pointer to value
  #else
  #define STBDS_ADDRESSOF(typevar, value)     ((typeof(typevar)[1]){value}) // literal array decays to pointer to value
  #endif
#else
#define STBDS_ADDRESSOF(typevar, value)     &(value)
#endif

#define STBDS_HM_BINARY 0
#define STBDS_HM_STRING 1

enum
{
   STBDS_SH_NONE,
   STBDS_SH_DEFAULT,
   STBDS_SH_STRDUP,
   STBDS_SH_ARENA
};

#define stbds_header(t)  ((stbds_array_header *) (t) - 1)

#define stbds_temp(t)    stbds_header(t)->temp

#define stbds_hmput(t, k, v) \
    ((t) = stbds_hmput_key_wrapper((t), sizeof *(t), (void*) STBDS_ADDRESSOF((t)->key, (k)), sizeof (t)->key, 0),   \
     (t)[stbds_temp((t)-1)].key = (k),    \
     (t)[stbds_temp((t)-1)].value = (v))

#define stbds_hmgeti(t,k) \
     ((t) = stbds_hmget_key_wrapper((t), sizeof *(t), (void*) STBDS_ADDRESSOF((t)->key, (k)), sizeof (t)->key, STBDS_HM_BINARY), \
       stbds_temp((t)-1))

#define stbds_hmlen(t)        ((t) ? (ptrdiff_t) stbds_header((t)-1)->length-1 : 0)
#define stbds_hmgetp_null(t,k)  (stbds_hmgeti(t,k) == -1 ? NULL : &(t)[stbds_temp((t)-1)])

#define stbds_hmput_key_wrapper           stbds_hmput_key
#define stbds_hmget_key_wrapper           stbds_hmget_key

void *stbds_hmput_key(void *a, size_t elemsize, void *key, size_t keysize, int mode);

void *stbds_hmget_key(void *a, size_t elemsize, void *key, size_t keysize, int mode);

#define hmput       stbds_hmput
#define hmgetp_null stbds_hmgetp_null
#define hmlen       stbds_hmlen

// ##### STB Hash Map #####

// ##### Dynamic Array #####

#define LIST_DEFAULT_CAPACITY 10

#define LIST_DEFINE(type, name) \
    typedef struct {            \
        type*  items;           \
        size_t count;           \
        size_t capacity;        \
    } name

void list_destroy_(void** items)
{
    free(*items);
    *items = NULL;
}

void list_alloc_(void** items, size_t* count, size_t* capacity, size_t typeSize, size_t startCapacity)
{
    if(*capacity >= startCapacity)
    {
        *count = 0;
        return;
    }
    else if(*capacity != 0)
        free(*items);
    
    *(char**)items = (char*)malloc(startCapacity * typeSize);
    *count = 0;
    *capacity = startCapacity;
}

void list_grow_(void** items, size_t* capacity, size_t minSpace, size_t typeSize)
{
    size_t prevCapacity = *capacity;
    *capacity *= 2;
	while(*capacity < minSpace) *capacity *= 2;

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
        list_grow_(items, capacity, *capacity + itemCount, itemsStride);
    
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

LIST_DEFINE(char, StringBuilder);

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

typedef struct {
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

bool filesystem_path_exists(const char* filepath)
{
    return access(filepath, F_OK) == 0;
}

struct stat filesystem_get_path_stats(const char* filepath)
{
    struct stat info;
    if(stat(filepath, &info) < 0)
    {
        printf("[ERROR] filesystem get last modified time: %s: %s\n", filepath, strerror(errno));
        return (struct stat) {0};
    }

    return info;
}

time_t filesystem_get_path_last_modified_time(const char* filepath)
{
    return filesystem_get_path_stats(filepath).st_mtime;
}

// ##### File #####

// ##### Directory #####

LIST_DEFINE(const char*, Files);

void directory_get_files_recursivelly(const char* dirpath, Files* files)
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
            directory_get_files_recursivelly(fullpath, files);
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

LIST_DEFINE(const char*, Dependencies);
LIST_DEFINE(const char*, Bld_Cmd);

typedef struct {
    uint32_t key;
    time_t   value;
} FilesCache;

typedef struct {
    uint32_t key;
    bool     value;
} ToUpdateMap;

typedef struct {
    Files sourcePaths;
	Files files;
	Files linkingFiles;
	bool mustRelink;
} Program;

static bool force = false;
static bool run   = false;

static int    runArgc = 0;
static char** runArgv = NULL;

static bool verbose = true;

static const char* _compiler = "cc";

static FilesCache*  filesCache  = NULL;
static ToUpdateMap* toUpdateMap = NULL;

static char tempStr[4096];

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
        return true;
    }

    Bld_Cmd cmdNull = {0};
    
    if(verbose)
    {
        printf("%s ", command);
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

bool bld_command_exec_and_reset(Bld_Cmd* cmd, const char* command)
{
    bool res = bld_command_exec(*cmd, command);
    cmd->count = 0;
    return res;
}

bool bld_command_run(Bld_Cmd cmd)
{
    if(!run)
        return true;
    
    for(int i = 0; i < runArgc; ++i)
        list_append(cmd, runArgv[i]);
    
    return bld_command_exec(cmd, "[RUN]");
}

bool bld_command_run_and_reset(Bld_Cmd* cmd)
{
    bool res = bld_command_run(*cmd);
    cmd->count = 0;
    return res;
}

void bld_save_files_cache()
{
    FILE* f = fopen("builder.cache", "wb");
    ptrdiff_t len = hmlen(filesCache);
    fwrite(&len, sizeof(ptrdiff_t), 1, f);
    fwrite(filesCache, len * sizeof(FilesCache), 1, f);
    fclose(f);
}

void bld_load_files_cache()
{
    FILE* f = fopen("builder.cache", "rb");
    if(!f)
        return;
    
    ptrdiff_t len;
    fread(&len, sizeof(ptrdiff_t), 1, f);

    FilesCache cache;
    for(ptrdiff_t i = 0; i < len; ++i)
    {
        fread(&cache, sizeof(FilesCache), 1, f);
        hmput(filesCache, cache.key, cache.value);
    }
    fclose(f);
}

void bld_create_vsc_files()
{
    if(!filesystem_path_exists(".vscode/c_cpp_properties.json"))
    {
        FILE* f = fopen(".vscode/c_cpp_properties.json", "w");
        const char text[] =
        "{\n"
        "    \"configurations\": [\n"
        "        {\n"
        "            \"name\": \"Linux\",\n"
        "            \"includePath\": [\n"
        "                \"${workspaceFolder}/src/\",\n"
        "                \"${workspaceFolder}/vendor/\"\n"
        "            ],\n"
        "            \"defines\": [ \"_BSD_SOURCE\" ],\n"
        "            \"compilerPath\": \"/usr/bin/gcc\",\n"
        "            \"cStandard\": \"c17\",\n"
        "            \"cppStandard\": \"c++17\",\n"
        "            \"intelliSenseMode\": \"linux-gcc-x64\"\n"
        "        }\n"
        "    ],\n"
        "    \"version\": 4\n"
        "}\n";
        fwrite(text, sizeof(text) - 1, 1, f);
        fclose(f);
    }
    if(!filesystem_path_exists(".vscode/launch.json"))
    {
        FILE* f = fopen(".vscode/launch.json", "w");
        const char text[] =
        "{\n"
        "    // Use IntelliSense to learn about possible attributes.\n"
        "    // Hover to view descriptions of existing attributes.\n"
        "    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387\n"
        "    \"version\": \"0.2.0\",\n"
        "    \"configurations\": [\n"
        "        {\n"
        "            \"name\": \"Builder\",\n"
        "            \"type\": \"cppdbg\",\n"
        "            \"request\": \"launch\",\n"
        "            \"program\": \"${workspaceFolder}/builder\",\n"
        "            \"cwd\": \"${workspaceFolder}\",\n"
        "        },\n"
        "        {\n"
        "            \"name\": \"Runner\",\n"
        "            \"type\": \"cppdbg\",\n"
        "            \"request\": \"launch\",\n"
        "            \"program\": \"${workspaceFolder}/builder\",\n"
        "            \"cwd\": \"${workspaceFolder}\",\n"
        "            \"args\": [ \"run\" ]\n"
        "        }\n"
        "    ]\n"
        "}\n";
        fwrite(text, sizeof(text) - 1, 1, f);
        fclose(f);
    }
    if(!filesystem_path_exists(".vscode/settings.json"))
    {
        FILE* f = fopen(".vscode/settings.json", "w");
        const char text[] =
        "{\n"
        "    \"files.associations\": {\n"
        "        \"builder.h\": \"c\",\n"
        "        \"stb_ds.h\": \"c\"\n"
        "    }\n"
        "}\n";
        fwrite(text, sizeof(text) - 1, 1, f);
        fclose(f);
    }
}

void bld_rebuild_yourself(const char* compiler, int* argc, char*** argv)
{
    if(!filesystem_path_exists("dependencies")) mkdir("dependencies", 0700);
    if(!filesystem_path_exists(".vscode")) mkdir(".vscode", 0700);
    if(!filesystem_path_exists("vendor")) mkdir("vendor", 0700);
    if(!filesystem_path_exists("build")) mkdir("build", 0700);
    if(!filesystem_path_exists("src")) mkdir("src", 0700);

    bld_create_vsc_files();

    const char* program = bld_shift_argument(argc, argv);
    
    if(*argc > 0 && strcmp(*argv[0], "force") == 0)
    {
        bld_shift_argument(argc, argv);
        force = true;
    }

    if(*argc > 0 && strcmp(*argv[0], "run") == 0)
    {
        bld_shift_argument(argc, argv);
        run = true;
    }

	_compiler = compiler;

    runArgc = *argc;
    runArgv = *argv;

    char cprogram[4 * 1024];
    sprintf(cprogram, "%s.c", program);

    struct stat cprogramStats = filesystem_get_path_stats(cprogram);

    // Rebuild
    if(cprogramStats.st_mtime != filesystem_get_path_last_modified_time(program))
    {
        Bld_Cmd cmd = {0};
        bld_command_append(&cmd, compiler, "-ggdb", "-Wall", "-Wextra", "-o", program, cprogram);
        if(!bld_command_exec(cmd, "[REBUILD]"))
            return;
        cmd.count = 0;

        struct utimbuf time;
        time.actime = cprogramStats.st_atime;
        time.modtime = cprogramStats.st_mtime;
        utime(program, &time);

        list_append(cmd, program);

        if(force)
        {
            const char* f = "force";
            list_append(cmd, f);
        }
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

	bld_load_files_cache();
}

char* string_file_path(const char* filepath)
{
    size_t len = strlen(filepath);
    for(ptrdiff_t i = len - 1; i >= 0; --i)
    {
        if(filepath[i] != '\\' && filepath[i] != '/')
            continue;
        
        ++i;
        char* path = malloc(i);
        memcpy(path, filepath, i);
        path[i] = '\0';
        return path;
    }
    return '\0';
}

char string_next_valid_char(const char* str, size_t len, size_t* offset)
{
    for(size_t i = 0; i < len; ++i)
        if(str[i] != ' ')
        {
            *offset = i;
            return str[i];
        }
    return '\0';
}

size_t string_index_of(const char* str, size_t len, char c)
{
    for(size_t i = 0; i < len; ++i)
        if(str[i] == c)
            return i;
    return -1;
}

void bld_get_file_dependencies(Files* sourcePaths, const char* filepath, Dependencies* dependencies)
{
    FILE* fp = fopen(filepath, "r");
    if(!fp) return;

    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    char* path = string_file_path(filepath);
    
    while ((read = getline(&line, &len, fp)) != -1)
    {
        size_t begin = 0;
        if(strncmp(line, "#include", 8) != 0 ||
            string_next_valid_char(line + sizeof("#include") - 1, read, &begin) == '<') continue;
        
        begin += sizeof("#include");
        size_t end = string_index_of(line + begin, read, '"') + begin;

        const char* f   = line + begin;
        size_t      len = end - begin;

        bool founded = false;
        for(size_t i = 0; i < sourcePaths->count; ++i)
        {
            sprintf(tempStr, "%s/%.*s", sourcePaths->items[i], (int)len, f);
            if(!filesystem_path_exists(tempStr))
                continue;

            char* file = realpath(tempStr, NULL);

            list_append(*dependencies, file);
            bld_get_file_dependencies(sourcePaths, file, dependencies);

            founded = true;
            break;
        }

        if(founded)
            continue;

        sprintf(tempStr, "%s/%.*s", path, (int)len, f);
        if(!filesystem_path_exists(tempStr))
        {
            printf("[ERROR] file dependency not found %.*s in %s\n", (int)len, f, filepath);
            continue;
        }
        
        char* file = realpath(tempStr, NULL);
        
        list_append(*dependencies, file);
        bld_get_file_dependencies(sourcePaths, file, dependencies);
    }

    if (line)
        free(line);
    fclose(fp);
}

char* bld_output_file(const char* file)
{
    const char* filename = builder_filename_no_extension(file);
    char* path = (char*) malloc(strlen(filename) + sizeof("build/.o"));
    sprintf(path, "build/%s.o", filename);
    return path;
}

uint32_t hash_str(const char* file, size_t len)
{
    const char* end = file + len;
    uint32_t hash = 2166136261;
    while(file != end)
    {
        hash ^= *(file++);
        hash *= 167776129;
    }
    return hash;
}

bool bld_file_must_update(const char* file, size_t len)
{
    uint32_t hash = hash_str(file, len);
    ToUpdateMap* updatePair = hmgetp_null(toUpdateMap, hash);
    if(updatePair == NULL)
    {
        FilesCache* cachePair = hmgetp_null(filesCache, hash);

        bool mustUpdate = true;
        if(cachePair == NULL)
            hmput(filesCache, hash, filesystem_get_path_last_modified_time(file));
        else
        {
            time_t t = filesystem_get_path_last_modified_time(file);
            mustUpdate = cachePair->value != t;
            if(mustUpdate)
                hmput(filesCache, hash, t);
        }
        
        hmput(toUpdateMap, hash, mustUpdate);
        return mustUpdate;
    }
    else
        return updatePair->value;
}

void bld_add_source_path(const char* sourceDir, Program* program)
{
    list_append(program->sourcePaths, sourceDir);
    directory_get_files_recursivelly(sourceDir, &program->files);
}

bool bld_compile_program(Program* program, Bld_Cmd cmd)
{
	for(size_t i = 0; i < program->files.count; ++i)
    {
        const char* file = program->files.items[i];
        size_t len = strlen(file);

        Dependencies dependencies = {0};
        bld_get_file_dependencies(&program->sourcePaths, file, &dependencies);

        const char* outFile = bld_output_file(file);
        list_append(program->linkingFiles, outFile);
        
        bool mustUpdate = force || bld_file_must_update(file, len);
        for(size_t j = 0; j < dependencies.count && !force; ++j)
        {
            const char* dependencie = dependencies.items[j];
            if(bld_file_must_update(dependencie, strlen(dependencie)))
            {
                mustUpdate = true;
                break;
            }
        }

        if(!mustUpdate)
            continue;

		program->mustRelink = true;
        
        bld_command_append(&cmd, "-c", file, "-o", outFile);
        sprintf(tempStr, "[%zu/%zu]", i + 1, program->files.count);
        if(!bld_command_exec(cmd, tempStr)) return false;

        cmd.count -= 4;
    }

	return true;
}

bool bld_link_program(Program* program, Bld_Cmd libraries, const char* outFile)
{
	Bld_Cmd cmd = {0};

	if(!program->mustRelink)
    {
        if(verbose)
            printf("Up to Date!\n");

		if(filesystem_path_exists(outFile))
		{
			sprintf(tempStr, "./%s", outFile);
			bld_command_append(&cmd, tempStr);
			
			if(!bld_command_run(cmd)) return false;
			return true;
		}
    }

	bld_command_append(&cmd, _compiler, "-o", outFile);
	
    for(size_t i = 0; i < program->linkingFiles.count; ++i)
        bld_command_append(&cmd, program->linkingFiles.items[i]);

	for(size_t i = 0; i < libraries.count; ++i)
        bld_command_append(&cmd, libraries.items[i]);
    
    if(!bld_command_exec_and_reset(&cmd, "[LINK]"))
        return false;
	
    sprintf(tempStr, "./%s", outFile);

    bld_command_append(&cmd, tempStr);
    if(!bld_command_run(cmd))
        return false;
    
    return true;
}

void bld_end()
{
	bld_save_files_cache();
}

// ##### Builder #####

// ##### STB Hash Map #####

#if defined(__GNUC__) || defined(__clang__)
#define STBDS_HAS_TYPEOF
#ifdef __cplusplus
//#define STBDS_HAS_LITERAL_ARRAY  // this is currently broken for clang
#endif
#endif

#if !defined(__cplusplus)
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define STBDS_HAS_LITERAL_ARRAY
#endif
#endif

#ifdef _MSC_VER
#define STBDS_NOTUSED(v)  (void)(v)
#else
#define STBDS_NOTUSED(v)  (void)sizeof(v)
#endif

#include <stdlib.h>
#define STBDS_REALLOC(c,p,s) realloc(p,s)
#define STBDS_FREE(c,p)      free(p)

#define stbds_temp_key(t) (*(char **) stbds_header(t)->hash_table)

#define stbds_arrcap(a)        ((a) ? stbds_header(a)->capacity : 0)
#define stbds_arrlen(a)        ((a) ? (ptrdiff_t) stbds_header(a)->length : 0)

#define STBDS_HASH_TO_ARR(x,elemsize) ((char*) (x) - (elemsize))
#define STBDS_ARR_TO_HASH(x,elemsize) ((char*) (x) + (elemsize))

#define STBDS_BUCKET_LENGTH 8

#define STBDS_BUCKET_SHIFT      (STBDS_BUCKET_LENGTH == 8 ? 3 : 2)
#define STBDS_BUCKET_MASK       (STBDS_BUCKET_LENGTH-1)
#define STBDS_CACHE_LINE_SIZE   64

#define STBDS_ALIGN_FWD(n,a)   (((n) + (a) - 1) & ~((a)-1))

#define STBDS_SIZE_T_BITS ((sizeof (size_t)) * 8)

#define STBDS_ROTATE_LEFT(val, n)  (((val) << (n)) | ((val) >> (STBDS_SIZE_T_BITS - (n))))
#define STBDS_ROTATE_RIGHT(val, n) (((val) >> (n)) | ((val) << (STBDS_SIZE_T_BITS - (n))))

#define STBDS_INDEX_EMPTY    -1
#define STBDS_INDEX_DELETED  -2
#define STBDS_INDEX_IN_USE(x)  ((x) >= 0)

#define STBDS_HASH_EMPTY      0
#define STBDS_HASH_DELETED    1

#define STBDS_SIPHASH_C_ROUNDS 1

#define STBDS_SIPHASH_D_ROUNDS 1

#define STBDS_STRING_ARENA_BLOCKSIZE_MIN  512u
#define STBDS_STRING_ARENA_BLOCKSIZE_MAX  (1u<<20)

#define stbds_hash_table(a)  ((stbds_hash_index *) stbds_header(a)->hash_table)

#define stbds_load_32_or_64(var, temp, v32, v64_hi, v64_lo)                                          \
  temp = v64_lo ^ v32, temp <<= 16, temp <<= 16, temp >>= 16, temp >>= 16, /* discard if 32-bit */   \
  var = v64_hi, var <<= 16, var <<= 16,                                    /* discard if 32-bit */   \
  var ^= temp ^ v32

static size_t stbds_hash_seed=0x31415926;

typedef struct stbds_string_block
{
  struct stbds_string_block *next;
  char storage[8];
} stbds_string_block;

typedef struct
{
  stbds_string_block *storage;
  size_t remaining;
  unsigned char block;
  unsigned char mode;  // this isn't used by the string arena itself
} stbds_string_arena;

typedef struct
{
   size_t    hash [STBDS_BUCKET_LENGTH];
   ptrdiff_t index[STBDS_BUCKET_LENGTH];
} stbds_hash_bucket; // in 32-bit, this is one 64-byte cache line; in 64-bit, each array is one 64-byte cache line

typedef struct
{
  char * temp_key; // this MUST be the first field of the hash table
  size_t slot_count;
  size_t used_count;
  size_t used_count_threshold;
  size_t used_count_shrink_threshold;
  size_t tombstone_count;
  size_t tombstone_count_threshold;
  size_t seed;
  size_t slot_count_log2;
  stbds_string_arena string;
  stbds_hash_bucket *storage; // not a separate allocation, just 64-byte aligned storage after this struct
} stbds_hash_index;

size_t stbds_hash_string(char *str, size_t seed)
{
  size_t hash = seed;
  while (*str)
     hash = STBDS_ROTATE_LEFT(hash, 9) + (unsigned char) *str++;

  // Thomas Wang 64-to-32 bit mix function, hopefully also works in 32 bits
  hash ^= seed;
  hash = (~hash) + (hash << 18);
  hash ^= hash ^ STBDS_ROTATE_RIGHT(hash,31);
  hash = hash * 21;
  hash ^= hash ^ STBDS_ROTATE_RIGHT(hash,11);
  hash += (hash << 6);
  hash ^= STBDS_ROTATE_RIGHT(hash,22);
  return hash+seed;
}

void *stbds_arrgrowf(void *a, size_t elemsize, size_t addlen, size_t min_cap)
{
  stbds_array_header temp={0}; // force debugging
  void *b;
  size_t min_len = stbds_arrlen(a) + addlen;
  (void) sizeof(temp);

  // compute the minimum capacity needed
  if (min_len > min_cap)
    min_cap = min_len;

  if (min_cap <= stbds_arrcap(a))
    return a;

  // increase needed capacity to guarantee O(1) amortized
  if (min_cap < 2 * stbds_arrcap(a))
    min_cap = 2 * stbds_arrcap(a);
  else if (min_cap < 4)
    min_cap = 4;

  //if (num_prev < 65536) if (a) prev_allocs[num_prev++] = (int *) ((char *) a+1);
  //if (num_prev == 2201)
  //  num_prev = num_prev;
  b = STBDS_REALLOC(NULL, (a) ? stbds_header(a) : 0, elemsize * min_cap + sizeof(stbds_array_header));
  //if (num_prev < 65536) prev_allocs[num_prev++] = (int *) (char *) b;
  b = (char *) b + sizeof(stbds_array_header);
  if (a == NULL) {
    stbds_header(b)->length = 0;
    stbds_header(b)->hash_table = 0;
    stbds_header(b)->temp = 0;
  }
  
  stbds_header(b)->capacity = min_cap;

  return b;
}

static size_t stbds_probe_position(size_t hash, size_t slot_count, size_t slot_log2)
{
  size_t pos;
  STBDS_NOTUSED(slot_log2);
  pos = hash & (slot_count-1);
  return pos;
}

static size_t stbds_log2(size_t slot_count)
{
  size_t n=0;
  while (slot_count > 1) {
    slot_count >>= 1;
    ++n;
  }
  return n;
}

static stbds_hash_index *stbds_make_hash_index(size_t slot_count, stbds_hash_index *ot)
{
  stbds_hash_index *t;
  t = (stbds_hash_index *) STBDS_REALLOC(NULL,0,(slot_count >> STBDS_BUCKET_SHIFT) * sizeof(stbds_hash_bucket) + sizeof(stbds_hash_index) + STBDS_CACHE_LINE_SIZE-1);
  t->storage = (stbds_hash_bucket *) STBDS_ALIGN_FWD((size_t) (t+1), STBDS_CACHE_LINE_SIZE);
  t->slot_count = slot_count;
  t->slot_count_log2 = stbds_log2(slot_count);
  t->tombstone_count = 0;
  t->used_count = 0;
  
  t->used_count_threshold        = slot_count - (slot_count>>2);
  t->tombstone_count_threshold   = (slot_count>>3) + (slot_count>>4);
  t->used_count_shrink_threshold = slot_count >> 2;
  
  if (slot_count <= STBDS_BUCKET_LENGTH)
    t->used_count_shrink_threshold = 0;
  // to avoid infinite loop, we need to guarantee that at least one slot is empty and will terminate probes
  if (ot) {
    t->string = ot->string;
    // reuse old seed so we can reuse old hashes so below "copy out old data" doesn't do any hashing
    t->seed = ot->seed;
  } else {
    size_t a,b,temp;
    memset(&t->string, 0, sizeof(t->string));
    t->seed = stbds_hash_seed;
    // LCG
    // in 32-bit, a =          2147001325   b =  715136305
    // in 64-bit, a = 2862933555777941757   b = 3037000493
    stbds_load_32_or_64(a,temp, 2147001325, 0x27bb2ee6, 0x87b0b0fd);
    stbds_load_32_or_64(b,temp,  715136305,          0, 0xb504f32d);
    stbds_hash_seed = stbds_hash_seed  * a + b;
  }

  {
    size_t i,j;
    for (i=0; i < slot_count >> STBDS_BUCKET_SHIFT; ++i) {
      stbds_hash_bucket *b = &t->storage[i];
      for (j=0; j < STBDS_BUCKET_LENGTH; ++j)
        b->hash[j] = STBDS_HASH_EMPTY;
      for (j=0; j < STBDS_BUCKET_LENGTH; ++j)
        b->index[j] = STBDS_INDEX_EMPTY;
    }
  }

  // copy out the old data, if any
  if (ot) {
    size_t i,j;
    t->used_count = ot->used_count;
    for (i=0; i < ot->slot_count >> STBDS_BUCKET_SHIFT; ++i) {
      stbds_hash_bucket *ob = &ot->storage[i];
      for (j=0; j < STBDS_BUCKET_LENGTH; ++j) {
        if (STBDS_INDEX_IN_USE(ob->index[j])) {
          size_t hash = ob->hash[j];
          size_t pos = stbds_probe_position(hash, t->slot_count, t->slot_count_log2);
          size_t step = STBDS_BUCKET_LENGTH;
          for (;;) {
            size_t limit,z;
            stbds_hash_bucket *bucket;
            bucket = &t->storage[pos >> STBDS_BUCKET_SHIFT];

            for (z=pos & STBDS_BUCKET_MASK; z < STBDS_BUCKET_LENGTH; ++z) {
              if (bucket->hash[z] == 0) {
                bucket->hash[z] = hash;
                bucket->index[z] = ob->index[j];
                goto done;
              }
            }

            limit = pos & STBDS_BUCKET_MASK;
            for (z = 0; z < limit; ++z) {
              if (bucket->hash[z] == 0) {
                bucket->hash[z] = hash;
                bucket->index[z] = ob->index[j];
                goto done;
              }
            }

            pos += step;                  // quadratic probing
            step += STBDS_BUCKET_LENGTH;
            pos &= (t->slot_count-1);
          }
        }
       done:
        ;
      }
    }
  }

  return t;
}

static size_t stbds_siphash_bytes(void *p, size_t len, size_t seed)
{
  unsigned char *d = (unsigned char *) p;
  size_t i,j;
  size_t v0,v1,v2,v3, data;
  
  v0 = ((((size_t) 0x736f6d65 << 16) << 16) + 0x70736575) ^  seed;
  v1 = ((((size_t) 0x646f7261 << 16) << 16) + 0x6e646f6d) ^ ~seed;
  v2 = ((((size_t) 0x6c796765 << 16) << 16) + 0x6e657261) ^  seed;
  v3 = ((((size_t) 0x74656462 << 16) << 16) + 0x79746573) ^ ~seed;
  

  #define STBDS_SIPROUND() \
    do {                   \
      v0 += v1; v1 = STBDS_ROTATE_LEFT(v1, 13);  v1 ^= v0; v0 = STBDS_ROTATE_LEFT(v0,STBDS_SIZE_T_BITS/2); \
      v2 += v3; v3 = STBDS_ROTATE_LEFT(v3, 16);  v3 ^= v2;                                                 \
      v2 += v1; v1 = STBDS_ROTATE_LEFT(v1, 17);  v1 ^= v2; v2 = STBDS_ROTATE_LEFT(v2,STBDS_SIZE_T_BITS/2); \
      v0 += v3; v3 = STBDS_ROTATE_LEFT(v3, 21);  v3 ^= v0;                                                 \
    } while (0)

  for (i=0; i+sizeof(size_t) <= len; i += sizeof(size_t), d += sizeof(size_t)) {
    data = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
    data |= (size_t) (d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24)) << 16 << 16; // discarded if size_t == 4

    v3 ^= data;
    for (j=0; j < STBDS_SIPHASH_C_ROUNDS; ++j)
      STBDS_SIPROUND();
    v0 ^= data;
  }
  data = len << (STBDS_SIZE_T_BITS-8);
  switch (len - i) {
    case 7: data |= ((size_t) d[6] << 24) << 24; // fall through
    case 6: data |= ((size_t) d[5] << 20) << 20; // fall through
    case 5: data |= ((size_t) d[4] << 16) << 16; // fall through
    case 4: data |= (d[3] << 24); // fall through
    case 3: data |= (d[2] << 16); // fall through
    case 2: data |= (d[1] << 8); // fall through
    case 1: data |= d[0]; // fall through
    case 0: break;
  }
  v3 ^= data;
  for (j=0; j < STBDS_SIPHASH_C_ROUNDS; ++j)
    STBDS_SIPROUND();
  v0 ^= data;
  v2 ^= 0xff;
  for (j=0; j < STBDS_SIPHASH_D_ROUNDS; ++j)
    STBDS_SIPROUND();
    
  return v1^v2^v3; // slightly stronger since v0^v3 in above cancels out final round operation? I tweeted at the authors of SipHash about this but they didn't reply

}

size_t stbds_hash_bytes(void *p, size_t len, size_t seed)
{
  unsigned char *d = (unsigned char *) p;

  if (len == 4) {
    unsigned int hash = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
    
    hash ^= seed;
    hash = (hash ^ 61) ^ (hash >> 16);
    hash = hash + (hash << 3);
    hash = hash ^ (hash >> 4);
    hash = hash * 0x27d4eb2d;
    hash ^= seed;
    hash = hash ^ (hash >> 15);
    
    return (((size_t) hash << 16 << 16) | hash) ^ seed;
  } else if (len == 8 && sizeof(size_t) == 8) {
    size_t hash = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
    hash |= (size_t) (d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24)) << 16 << 16; // avoid warning if size_t == 4
    hash ^= seed;
    hash = (~hash) + (hash << 21);
    hash ^= STBDS_ROTATE_RIGHT(hash,24);
    hash *= 265;
    hash ^= STBDS_ROTATE_RIGHT(hash,14);
    hash ^= seed;
    hash *= 21;
    hash ^= STBDS_ROTATE_RIGHT(hash,28);
    hash += (hash << 31);
    hash = (~hash) + (hash << 18);
    return hash;
  } else {
    return stbds_siphash_bytes(p,len,seed);
  }
}

static int stbds_is_key_equal(void *a, size_t elemsize, void *key, size_t keysize, size_t keyoffset, int mode, size_t i)
{
  if (mode >= STBDS_HM_STRING)
    return 0==strcmp((char *) key, * (char **) ((char *) a + elemsize*i + keyoffset));
  else
    return 0==memcmp(key, (char *) a + elemsize*i + keyoffset, keysize);
}

static char *stbds_strdup(char *str)
{
  // to keep replaceable allocator simple, we don't want to use strdup.
  // rolling our own also avoids problem of strdup vs _strdup
  size_t len = strlen(str)+1;
  char *p = (char*) STBDS_REALLOC(NULL, 0, len);
  memmove(p, str, len);
  return p;
}

char *stbds_stralloc(stbds_string_arena *a, char *str)
{
  char *p;
  size_t len = strlen(str)+1;
  if (len > a->remaining) {
    // compute the next blocksize
    size_t blocksize = a->block;

    // size is 512, 512, 1024, 1024, 2048, 2048, 4096, 4096, etc., so that
    // there are log(SIZE) allocations to free when we destroy the table
    blocksize = (size_t) (STBDS_STRING_ARENA_BLOCKSIZE_MIN) << (blocksize>>1);

    // if size is under 1M, advance to next blocktype
    if (blocksize < (size_t)(STBDS_STRING_ARENA_BLOCKSIZE_MAX))
      ++a->block;

    if (len > blocksize) {
      // if string is larger than blocksize, then just allocate the full size.
      // note that we still advance string_block so block size will continue
      // increasing, so e.g. if somebody only calls this with 1000-long strings,
      // eventually the arena will start doubling and handling those as well
      stbds_string_block *sb = (stbds_string_block *) STBDS_REALLOC(NULL, 0, sizeof(*sb)-8 + len);
      memmove(sb->storage, str, len);
      if (a->storage) {
        // insert it after the first element, so that we don't waste the space there
        sb->next = a->storage->next;
        a->storage->next = sb;
      } else {
        sb->next = 0;
        a->storage = sb;
        a->remaining = 0; // this is redundant, but good for clarity
      }
      return sb->storage;
    } else {
      stbds_string_block *sb = (stbds_string_block *) STBDS_REALLOC(NULL, 0, sizeof(*sb)-8 + blocksize);
      sb->next = a->storage;
      a->storage = sb;
      a->remaining = blocksize;
    }
  }

  p = a->storage->storage + a->remaining - len;
  a->remaining -= len;
  memmove(p, str, len);
  return p;
}

void *stbds_hmput_key(void *a, size_t elemsize, void *key, size_t keysize, int mode)
{
  size_t keyoffset=0;
  void *raw_a;
  stbds_hash_index *table;

  if (a == NULL) {
    a = stbds_arrgrowf(0, elemsize, 0, 1);
    memset(a, 0, elemsize);
    stbds_header(a)->length += 1;
    // adjust a to point AFTER the default element
    a = STBDS_ARR_TO_HASH(a,elemsize);
  }

  // adjust a to point to the default element
  raw_a = a;
  a = STBDS_HASH_TO_ARR(a,elemsize);

  table = (stbds_hash_index *) stbds_header(a)->hash_table;

  if (table == NULL || table->used_count >= table->used_count_threshold) {
    stbds_hash_index *nt;
    size_t slot_count;

    slot_count = (table == NULL) ? STBDS_BUCKET_LENGTH : table->slot_count*2;
    nt = stbds_make_hash_index(slot_count, table);
    if (table)
      STBDS_FREE(NULL, table);
    else
      nt->string.mode = mode >= STBDS_HM_STRING ? STBDS_SH_DEFAULT : 0;
    stbds_header(a)->hash_table = table = nt;
  }

  // we iterate hash table explicitly because we want to track if we saw a tombstone
  {
    size_t hash = mode >= STBDS_HM_STRING ? stbds_hash_string((char*)key,table->seed) : stbds_hash_bytes(key, keysize,table->seed);
    size_t step = STBDS_BUCKET_LENGTH;
    size_t pos;
    ptrdiff_t tombstone = -1;
    stbds_hash_bucket *bucket;

    // stored hash values are forbidden from being 0, so we can detect empty slots to early out quickly
    if (hash < 2) hash += 2;

    pos = stbds_probe_position(hash, table->slot_count, table->slot_count_log2);

    for (;;) {
      size_t limit, i;
      bucket = &table->storage[pos >> STBDS_BUCKET_SHIFT];

      // start searching from pos to end of bucket
      for (i=pos & STBDS_BUCKET_MASK; i < STBDS_BUCKET_LENGTH; ++i) {
        if (bucket->hash[i] == hash) {
          if (stbds_is_key_equal(raw_a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
            stbds_temp(a) = bucket->index[i];
            if (mode >= STBDS_HM_STRING)
              stbds_temp_key(a) = * (char **) ((char *) raw_a + elemsize*bucket->index[i] + keyoffset);
            return STBDS_ARR_TO_HASH(a,elemsize);
          }
        } else if (bucket->hash[i] == 0) {
          pos = (pos & ~STBDS_BUCKET_MASK) + i;
          goto found_empty_slot;
        } else if (tombstone < 0) {
          if (bucket->index[i] == STBDS_INDEX_DELETED)
            tombstone = (ptrdiff_t) ((pos & ~STBDS_BUCKET_MASK) + i);
        }
      }

      // search from beginning of bucket to pos
      limit = pos & STBDS_BUCKET_MASK;
      for (i = 0; i < limit; ++i) {
        if (bucket->hash[i] == hash) {
          if (stbds_is_key_equal(raw_a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
            stbds_temp(a) = bucket->index[i];
            return STBDS_ARR_TO_HASH(a,elemsize);
          }
        } else if (bucket->hash[i] == 0) {
          pos = (pos & ~STBDS_BUCKET_MASK) + i;
          goto found_empty_slot;
        } else if (tombstone < 0) {
          if (bucket->index[i] == STBDS_INDEX_DELETED)
            tombstone = (ptrdiff_t) ((pos & ~STBDS_BUCKET_MASK) + i);
        }
      }

      // quadratic probing
      pos += step;
      step += STBDS_BUCKET_LENGTH;
      pos &= (table->slot_count-1);
    }
   found_empty_slot:
    if (tombstone >= 0) {
      pos = tombstone;
      --table->tombstone_count;
    }
    ++table->used_count;

    {
      ptrdiff_t i = (ptrdiff_t) stbds_arrlen(a);
      // we want to do stbds_arraddn(1), but we can't use the macros since we don't have something of the right type
      if ((size_t) i+1 > stbds_arrcap(a))
        *(void **) &a = stbds_arrgrowf(a, elemsize, 1, 0);
      raw_a = STBDS_ARR_TO_HASH(a,elemsize);

      stbds_header(a)->length = i+1;
      bucket = &table->storage[pos >> STBDS_BUCKET_SHIFT];
      bucket->hash[pos & STBDS_BUCKET_MASK] = hash;
      bucket->index[pos & STBDS_BUCKET_MASK] = i-1;
      stbds_temp(a) = i-1;

      switch (table->string.mode) {
         case STBDS_SH_STRDUP:  stbds_temp_key(a) = *(char **) ((char *) a + elemsize*i) = stbds_strdup((char*) key); break;
         case STBDS_SH_ARENA:   stbds_temp_key(a) = *(char **) ((char *) a + elemsize*i) = stbds_stralloc(&table->string, (char*)key); break;
         case STBDS_SH_DEFAULT: stbds_temp_key(a) = *(char **) ((char *) a + elemsize*i) = (char *) key; break;
         default:                memcpy((char *) a + elemsize*i, key, keysize); break;
      }
    }
    return STBDS_ARR_TO_HASH(a,elemsize);
  }
}

void * stbds_shmode_func(size_t elemsize, int mode)
{
  void *a = stbds_arrgrowf(0, elemsize, 0, 1);
  stbds_hash_index *h;
  memset(a, 0, elemsize);
  stbds_header(a)->length = 1;
  stbds_header(a)->hash_table = h = (stbds_hash_index *) stbds_make_hash_index(STBDS_BUCKET_LENGTH, NULL);
  h->string.mode = (unsigned char) mode;
  return STBDS_ARR_TO_HASH(a,elemsize);
}

static ptrdiff_t stbds_hm_find_slot(void *a, size_t elemsize, void *key, size_t keysize, size_t keyoffset, int mode)
{
  void *raw_a = STBDS_HASH_TO_ARR(a,elemsize);
  stbds_hash_index *table = stbds_hash_table(raw_a);
  size_t hash = mode >= STBDS_HM_STRING ? stbds_hash_string((char*)key,table->seed) : stbds_hash_bytes(key, keysize,table->seed);
  size_t step = STBDS_BUCKET_LENGTH;
  size_t limit,i;
  size_t pos;
  stbds_hash_bucket *bucket;

  if (hash < 2) hash += 2; // stored hash values are forbidden from being 0, so we can detect empty slots

  pos = stbds_probe_position(hash, table->slot_count, table->slot_count_log2);

  for (;;) {
    bucket = &table->storage[pos >> STBDS_BUCKET_SHIFT];

    // start searching from pos to end of bucket, this should help performance on small hash tables that fit in cache
    for (i=pos & STBDS_BUCKET_MASK; i < STBDS_BUCKET_LENGTH; ++i) {
      if (bucket->hash[i] == hash) {
        if (stbds_is_key_equal(a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
          return (pos & ~STBDS_BUCKET_MASK)+i;
        }
      } else if (bucket->hash[i] == STBDS_HASH_EMPTY) {
        return -1;
      }
    }

    // search from beginning of bucket to pos
    limit = pos & STBDS_BUCKET_MASK;
    for (i = 0; i < limit; ++i) {
      if (bucket->hash[i] == hash) {
        if (stbds_is_key_equal(a, elemsize, key, keysize, keyoffset, mode, bucket->index[i])) {
          return (pos & ~STBDS_BUCKET_MASK)+i;
        }
      } else if (bucket->hash[i] == STBDS_HASH_EMPTY) {
        return -1;
      }
    }

    // quadratic probing
    pos += step;
    step += STBDS_BUCKET_LENGTH;
    pos &= (table->slot_count-1);
  }
  /* NOTREACHED */
}

void * stbds_hmget_key_ts(void *a, size_t elemsize, void *key, size_t keysize, ptrdiff_t *temp, int mode)
{
  size_t keyoffset = 0;
  if (a == NULL) {
    // make it non-empty so we can return a temp
    a = stbds_arrgrowf(0, elemsize, 0, 1);
    stbds_header(a)->length += 1;
    memset(a, 0, elemsize);
    *temp = STBDS_INDEX_EMPTY;
    // adjust a to point after the default element
    return STBDS_ARR_TO_HASH(a,elemsize);
  } else {
    stbds_hash_index *table;
    void *raw_a = STBDS_HASH_TO_ARR(a,elemsize);
    // adjust a to point to the default element
    table = (stbds_hash_index *) stbds_header(raw_a)->hash_table;
    if (table == 0) {
      *temp = -1;
    } else {
      ptrdiff_t slot = stbds_hm_find_slot(a, elemsize, key, keysize, keyoffset, mode);
      if (slot < 0) {
        *temp = STBDS_INDEX_EMPTY;
      } else {
        stbds_hash_bucket *b = &table->storage[slot >> STBDS_BUCKET_SHIFT];
        *temp = b->index[slot & STBDS_BUCKET_MASK];
      }
    }
    return a;
  }
}

void * stbds_hmget_key(void *a, size_t elemsize, void *key, size_t keysize, int mode)
{
  ptrdiff_t temp;
  void *p = stbds_hmget_key_ts(a, elemsize, key, keysize, &temp, mode);
  stbds_temp(STBDS_HASH_TO_ARR(p,elemsize)) = temp;
  return p;
}

// ##### STB Hash Map #####

#endif // BUILDER_H_

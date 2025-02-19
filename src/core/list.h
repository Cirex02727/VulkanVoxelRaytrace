#ifndef LIST_H_
#define LIST_H_

#include <stdbool.h>
#include <stddef.h>

/*  
 *  USAGE:
 *  
 *  typedef struct {
 *      int* items;
 *      size_t count;
 *      size_t capacity;
 *  } List;
 *  
 *  List l;
 *  list_alloc(l);
 *  
 *  int n = 5;
 *  list_append(l, n);
 *  n = 10;
 *  list_append(l, n);
 *  
 *  for(size_t i = 0; i < list_count(l); i++)
 *      printf("%zu: %d\n", i, l.items[i]);
 */

#define LIST_DEFAULT_CAPACITY 10

#define LIST_DEFINE(type, name) \
    typedef struct {            \
        type*  items;           \
        size_t count;           \
        size_t capacity;        \
    } name

void list_alloc_(void** items, size_t* count, size_t* capacity, size_t typeSize, size_t startCapacity);

void list_destroy_(void** items);

void list_grow_(void** items, size_t* capacity, size_t typeSize);

void* list_append_(void** items, size_t* count, size_t* capacity, void* item, size_t itemCount, size_t typeSize);

#define list_alloc(list, startCapacity) list_alloc_((void**)&(list).items, &(list).count, &(list).capacity, sizeof((list).items[0]), startCapacity)

#define list_alloc_default(list) list_alloc(list, LIST_DEFAULT_CAPACITY)

#define list_alloc_sized(list, typeSize, startCapacity) list_alloc_((void**)&(list).items, &(list).count, &(list).capacity, typeSize, startCapacity)

#define list_alloc_sized_default(list, typeSize) list_alloc_sized(list, typeSize, LIST_DEFAULT_CAPACITY)


#define list_destroy(list) list_destroy_((void**)&(list).items);


#define list_append(list, item) list_append_((void**)&(list).items, &(list).count, &(list).capacity, &item, 1, sizeof(*(list).items))

#define list_append_multiple(list, item, itemCount) list_append_((void**)&(list).items, &(list).count, &(list).capacity, &item, itemCount, sizeof(*(list).items))

#define list_append_sized(list, item, typeSize) list_append_((void**)&(list).items, &(list).count, &(list).capacity, item, 1, typeSize)

#define list_append_sized_multiple(list, item, itemCount, typeSize) list_append_((void**)&(list).items, &(list).count, &(list).capacity, item, itemCount, typeSize)


// ##### Set #####

void list_append_unique_(void** items, size_t* count, size_t* capacity, void* item, size_t itemCount, size_t typeSize);

#define list_append_unique(list, item) list_append_unique_((void**)&(list).items, &(list).count, &(list).capacity, &item, 1, sizeof(*(list).items))

// ##### Set #####

// ##### Utils #####

bool list_contains_(void* items, size_t count, void* item, size_t itemCount, size_t typeSize);

#define list_contains(list, item) list_contains_((list).items, (list).count, &item, 1, sizeof(*(list).items))

// ##### Utils #####

#endif  //LIST_H_

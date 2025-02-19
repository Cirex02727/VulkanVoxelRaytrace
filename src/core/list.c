#include "list.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

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

// ##### Set #####

void list_append_unique_(void** items, size_t* count, size_t* capacity, void* item, size_t itemCount, size_t typeSize)
{
    if(list_contains_(*items, *count, item, itemCount, typeSize))
        return;
    
    list_append_(items, count, capacity, item, itemCount, typeSize);
}

// ##### Set #####

// ##### Utils #####

bool list_contains_(void* items, size_t count, void* item, size_t itemCount, size_t typeSize)
{
    itemCount *= typeSize;

    for(size_t i = 0; i < count; ++i)
    {
        void* ptr = items + i * typeSize;
        if(memcmp(ptr, item, itemCount) == 0)
            return true;
    }

    return false;
}

// ##### Utils #####

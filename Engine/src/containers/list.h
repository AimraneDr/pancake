#pragma once

#include "defines.h"


/*
Memory layout
u64 capacity = number elements that can be held
u64 length = number of elements currently contained
u64 stride = size of each element in bytes
void* elements
*/
enum{
    LIST_CAPACITY,
    LIST_LENGTH,
    LIST_STRIDE,
    LIST_FIELDS_LENGTH
};

PANCAKE_API void* _list_create(u64 length, u64 stride);
PANCAKE_API void _list_destroy(void* list);

PANCAKE_API u64 _list_field_get(void* list, u64 field);
PANCAKE_API void _list_field_set(void* list, u64 field, u64 value);

PANCAKE_API void* _list_resize(void* list);

PANCAKE_API void* _list_push(void* list, const void* value_ptr);
PANCAKE_API void _list_pop(void* list, void* dest);

PANCAKE_API void* _list_insert(void* list, u64 index, const void* value_ptr);
PANCAKE_API void* _list_pop_at(void* list, u64 index, void* dest);

#define LIST_DEFAULT_CAPACITY 1
#define LIST_RESIZE_FACTOR 2

#define list_create(type) \
    _list_create(LIST_DEFAULT_CAPACITY, sizeof(type))

#define list_reserve(type, capacity) \
    _list_create(capacity, sizeof(type))

#define list_destroy(list) _list_destroy(list);

#define list_push(list, value)           \
    {                                       \
        typeof(value) temp = value;         \
        list = _list_push(list, &temp); \
    }
// NOTE: could use __auto_type for temp above, but intellisense
// for VSCode flags it as an unknown type. typeof() seems to
// work just fine, though. Both are GNU extensions.

#define list_pop(list, value_ptr) \
    _list_pop(list, value_ptr)

#define list_insert(list, index, value)           \
    {                                             \
        typeof(value) temp = value;               \
        list = _list_insert(list, index, &temp);  \
    }

#define list_pop_at(list, index, value_ptr) \
    _list_pop_at(list, index, value_ptr)

#define list_clear(list) \
    _list_field_set(list, LIST_LENGTH, 0)

#define list_capacity(list) \
    _list_field_get(list, LIST_CAPACITY)

#define list_length(list) \
    _list_field_get(list, LIST_LENGTH)

#define list_stride(list) \
    _list_field_get(list, LIST_STRIDE)

#define list_length_set(list, value) \
    _list_field_set(list, LIST_LENGTH, value)
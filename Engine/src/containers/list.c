#include "list.h"
#include "core/pancake_memory.h"
#include "core/logger.h"

void* _list_create(u64 length, u64 stride) {
    u64 header_size = LIST_FIELDS_LENGTH * sizeof(u64);
    u64 list_size = length * stride;
    u64* new_list = pancake_allocate(header_size + list_size, MEMORY_TAG_LIST);
    pancake_set_memory(new_list, 0, header_size + list_size);
    new_list[LIST_CAPACITY] = length;
    new_list[LIST_LENGTH] = 0;
    new_list[LIST_STRIDE] = stride;
    return (void*)(new_list + LIST_FIELDS_LENGTH);
}
PANCAKE_API void _list_destroy(void* list){
    u64* header = (u64*)list - LIST_FIELDS_LENGTH;
    u64 header_size = LIST_FIELDS_LENGTH * sizeof(u64);
    u64 total_size = header_size + header[LIST_CAPACITY] * header[LIST_STRIDE];
    pancake_free(header, total_size, MEMORY_TAG_LIST);
}

PANCAKE_API u64 _list_field_get(void* list, u64 field){
     u64* header = (u64*)list - LIST_FIELDS_LENGTH;
    return header[field];
}
PANCAKE_API void _list_field_set(void* list, u64 field, u64 value){
    u64* header = (u64*)list - LIST_FIELDS_LENGTH;
    header[field] = value;
}

PANCAKE_API void* _list_resize(void* list){
    u64 length = list_length(list);
    u64 stride = list_stride(list);
    void* temp = _list_create(
        (LIST_RESIZE_FACTOR * list_capacity(list)),
        stride);
    pancake_copy_memory(temp, list, length * stride);

    _list_field_set(temp, LIST_LENGTH, length);
    _list_destroy(list);
    return temp;
}

PANCAKE_API void* _list_push(void* list, const void* value_ptr){
    u64 length = list_length(list);
    u64 stride = list_stride(list);
    if (length >= list_capacity(list)) {
        list = _list_resize(list);
    }

    u64 addr = (u64)list;
    addr += (length * stride);
    pancake_copy_memory((void*)addr, value_ptr, stride);
    _list_field_set(list, LIST_LENGTH, length + 1);
    return list;
}
PANCAKE_API void _list_pop(void* list, void* dest){
    u64 length = list_length(list);
    u64 stride = list_stride(list);
    u64 addr = (u64)list;
    addr += ((length - 1) * stride);
    pancake_copy_memory(dest, (void*)addr, stride);
    _list_field_set(list, LIST_LENGTH, length - 1);
}

PANCAKE_API void* _list_insert(void* list, u64 index, const void* value_ptr){
    u64 length = list_length(list);
    u64 stride = list_stride(list);
    if (index >= length) {
        PANCAKE_ERROR("Index outside the bounds of this list! Length: %i, index: %index", length, index);
        return list;
    }
    if (length >= list_capacity(list)) {
        list = _list_resize(list);
    }

    u64 addr = (u64)list;

    // If not on the last element, copy the rest outward.
    if (index != length - 1) {
        pancake_copy_memory(
            (void*)(addr + ((index + 1) * stride)),
            (void*)(addr + (index * stride)),
            stride * (length - index));
    }

    // Set the value at the index
    pancake_copy_memory((void*)(addr + (index * stride)), value_ptr, stride);

    _list_field_set(list, LIST_LENGTH, length + 1);
    return list;
}
PANCAKE_API void* _list_pop_at(void* list, u64 index, void* dest){
    u64 length = list_length(list);
    u64 stride = list_stride(list);
    if (index >= length) {
        PANCAKE_ERROR("Index outside the bounds of this list! Length: %i, index: %index", length, index);
        return list;
    }

    u64 addr = (u64)list;
    pancake_copy_memory(dest, (void*)(addr + (index * stride)), stride);

    // If not on the last element, snip out the entry and copy the rest inward.
    if (index != length - 1) {
        pancake_copy_memory(
            (void*)(addr + (index * stride)),
            (void*)(addr + ((index + 1) * stride)),
            stride * (length - index));
    }

    _list_field_set(list, LIST_LENGTH, length - 1);
    return list;
}
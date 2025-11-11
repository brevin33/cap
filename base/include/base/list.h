#pragma once

#define create_list_impl(ListType, ElemType)                              \
    void ListType##_add(ListType* list, ElemType* elem) {                 \
        if (list->capacity == 0) {                                        \
            list->capacity = 8;                                           \
            list->data = alloc(list->capacity * sizeof(ElemType));        \
        }                                                                 \
        if (list->count >= list->capacity) {                              \
            list->capacity *= 2;                                          \
            void* old_data = list->data;                                  \
            list->data = alloc(list->capacity * sizeof(ElemType));        \
            memcpy(list->data, old_data, list->count * sizeof(ElemType)); \
        }                                                                 \
        list->data[list->count] = *elem;                                  \
        list->count++;                                                    \
    }                                                                     \
    ElemType* ListType##_get(ListType* list, u64 index) {                 \
        return &list->data[index];                                        \
    }

#define create_list_headers(ListType, ElemType)          \
    typedef struct ListType {                            \
        ElemType* data;                                  \
        u64 count;                                       \
        u64 capacity;                                    \
    } ListType;                                          \
    void ListType##_add(ListType* list, ElemType* elem); \
    ElemType* ListType##_get(ListType* list, u64 index);

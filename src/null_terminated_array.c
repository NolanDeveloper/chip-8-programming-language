#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "null_terminated_array.h"

#define ASSERT_NOT_NULL(x) if (!(x)) { return NTA_BAD_ARGUMENT; }
#define ASSERT_NOT_ZERO(x) ASSERT_NON_NULL(x)

extern enum NtaError 
null_terminated_array_init(
        struct NullTerminatedArray *nta, size_t capacity, size_t element_size, 
        void *null, void *(*allocate)(size_t)) {
    size_t size     = capacity * element_size;
    void *data      = allocate(size + element_size);
    void *_null     = (char*)data + size;
    if (!data) return NTA_OUT_OF_MEMORY;
    memcpy(_null, null, element_size);
    memcpy(data, null, element_size);
    *nta = (struct NullTerminatedArray) {
        .data           = data,
        .null           = _null,
        .element_size   = element_size,
        .capacity       = capacity,
    };
    return NTA_OK;
}

extern void
null_terminated_array_free(struct NullTerminatedArray *nta, void (*free)(void *)) {
    free(nta->data);
}

extern void
null_terminated_array_add(struct NullTerminatedArray *nta, void *element) {
    memmove(
        (char *)nta->data + nta->element_size, 
        nta->data, 
        (nta->capacity - 2) * nta->element_size);
    memcpy(nta->data, element, nta->element_size);
}

extern void 
null_terminated_array_remove(struct NullTerminatedArray *nta, void *element) {
    char *in = nta->data;
    char *out = in; 
    for (; memcmp(in, nta->null, nta->element_size); in += nta->element_size) {
        if (!memcmp(in, element, nta->element_size)) continue;
        memcpy(out, in, nta->element_size);
        out += nta->element_size;
    }
    memcpy(out, nta->null, nta->element_size);
}

extern bool
null_terminated_array_is_empty(struct NullTerminatedArray *nta) {
    return !memcmp(nta->data, nta->null, nta->element_size);
}

extern size_t
null_terminated_array_size(struct NullTerminatedArray *nta) {
    size_t size = 0;
    char *p = nta->data;
    while (memcmp(p, nta->null, nta->element_size)) { 
        p += nta->element_size;
        ++size;
    }
    return size;
}

extern void *
null_terminated_array_at(struct NullTerminatedArray *nta, size_t i) {
    return (char *)nta->data + i * nta->element_size;
}

#ifdef TESTING

#include <stdio.h>

#define ARRAY_SIZE 4

static struct NullTerminatedArray array;

static void
t_initialize(void) {
    int zero = 0;
    null_terminated_array_init(&array, ARRAY_SIZE, sizeof(int), &zero, malloc);
}

static void
t_finalize(void) {
    null_terminated_array_free(&array, free);
}

static int
t_at(size_t i) {
    return *(int *)null_terminated_array_at(&array, i);
}

static void
t_add(int x) {
    null_terminated_array_add(&array, &x);
}

static void
t_remove(int x) {
    null_terminated_array_remove(&array, &x);
}

static void
t_check_at(size_t position, int expected_value) {
    int actual = t_at(position);
    if (actual == expected_value) return;
    exit(1);
}

static void
t_check_content(int a, int b, int c, int d) {
    t_check_at(0, a);
    t_check_at(1, b);
    t_check_at(2, c);
    t_check_at(3, d);
}

int main() {
    t_initialize();         t_check_content(0, 0, 0, 0);
    t_add(1);               t_check_content(1, 0, 0, 0);
    t_add(2);               t_check_content(2, 1, 0, 0);
    t_add(3);               t_check_content(3, 2, 1, 0);
    t_add(4);               t_check_content(4, 3, 2, 0);
    t_remove(2);            t_check_content(4, 3, 0, 0);
    t_add(5);               t_check_content(5, 4, 3, 0);
    t_add(6);               t_check_content(6, 5, 4, 0);
    t_remove(5);            t_check_content(6, 4, 0, 0);
    t_remove(4);            t_check_content(6, 0, 0, 0);
    t_remove(6);            t_check_content(0, 0, 0, 0);
    t_finalize();
}

#endif

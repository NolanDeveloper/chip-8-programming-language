#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tiny_set.h"

#define ALLOCATE    malloc
#define FREE        free

int 
tiny_set_init(struct TinySet *ts, size_t capacity, size_t element_size) {
    void *data = ALLOCATE(capacity * element_size);
    if (!data) return 1;
    *ts = (struct TinySet) {
        .data           = data,
        .element_size   = element_size,
        .size           = 0,
        .capacity       = capacity,
    };
    return 0;
}

void 
tiny_set_free(struct TinySet *ts) {
    FREE(ts->data);
}

void *
tiny_set_lookup(struct TinySet *ts, void *element) {
    size_t element_size = ts->element_size;
    char *p = (char *)ts->data;
    char *end = p + ts->size * element_size;
    while (p != end && memcmp(p, element, element_size)) {
        p += ts->element_size;
    }
    return p == end ? NULL : p;
}

void 
tiny_set_remove(struct TinySet *ts, void *element) {
    void *p = tiny_set_lookup(ts, element);
    if (!p) return;
    char *end = (char *)ts->data + ts->capacity * ts->element_size;
    memmove(p, (char *)p + ts->element_size, end - ((char *)p + ts->element_size));
    --ts->size;
}

void
tiny_set_add(struct TinySet *ts, void *element) {
    if (tiny_set_lookup(ts, element)) return;
    memmove(
        (char *)ts->data + ts->element_size, 
        ts->data, 
        (ts->capacity - 1) * ts->element_size);
    memcpy(ts->data, element, ts->element_size);
    if (ts->size != ts->capacity) ++ts->size;
}

size_t
tiny_set_size(struct TinySet *ts) {
    return ts->size;
}

#ifdef TESTING

struct TinySet ts;

void t_init(void)       { tiny_set_init(&ts, 4, sizeof(int)); }
void t_finish(void)     { tiny_set_free(&ts); }
void t_add(int x)       { tiny_set_add(&ts, &x); }
void t_remove(int x)    { tiny_set_remove(&ts, &x); }
size_t t_size()         { return tiny_set_size(&ts); }
bool t_lookup(int x)    { return tiny_set_lookup(&ts, &x); }

#include <stdio.h>
#include <stdarg.h>

#define ASSERT_MSG(c, ...) \
    if (!(c)) { \
        printf(__FILE__ ":%i: (" #c ") is not true.\n", __LINE__); \
        printf(__VA_ARGS__); \
        exit(1); \
    }

void t_check(size_t n, ...) { 
    va_list args;
    ASSERT_MSG(n == t_size(), "t_size() = %zu\n", t_size())
    va_start(args, n);
    for (size_t i = 0; i < n; ++i) {
        int x = va_arg(args, int);
        ASSERT_MSG(t_lookup(x), "x = %i\n", x);
    }
    va_end(args);
}

int main() {
    t_init(); t_check(0);
    t_add(1); t_check(1, 1);
    t_add(2); t_check(2, 1, 2);
    t_add(3); t_check(3, 1, 2, 3);
    t_add(4); t_check(4, 1, 2, 3, 4);
    t_add(5); t_check(4, 2, 3, 4, 5);
    t_remove(6); t_check(4, 2, 3, 4, 5);
    t_remove(5); t_check(3, 2, 3, 4);
    t_remove(2); t_check(2, 3, 4);
    t_add(7); t_check(3, 3, 4, 7);
    t_remove(3); t_check(2, 4, 7);
    t_remove(4); t_check(1, 7);
    t_remove(7); t_check(0);
    t_remove(7); t_check(0);
    t_finish();
}

#endif

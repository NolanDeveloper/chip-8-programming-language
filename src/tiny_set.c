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

#if TESTING

struct TinySet ts;

void t_init(void)       { tiny_set_init(&ts, 4, sizeof(int)); }
void t_finish(void)     { tiny_set_free(&ts); }
void t_add(int x)       { tiny_set_add(&ts, &x); }
void t_remove(int x)    { tiny_set_remove(&ts, &x); }
size_t t_size()         { return tiny_set_size(&ts); }
bool t_lookup(int x)    { return tiny_set_lookup(&ts, &x); }

#include <stdio.h>

#define ASSERT(c) if (!(c)) { printf(__FILE__ ":%i: (" #c ") is not true.\n", __LINE__); exit(1); }

int main() {
    t_init();
    ASSERT(0 == t_size())
    t_add(1);
    ASSERT(1 == t_size())
    ASSERT(t_lookup(1))
    t_add(2);
    ASSERT(2 == t_size())
    ASSERT(t_lookup(1))
    ASSERT(t_lookup(2))
    t_add(3);
    ASSERT(3 == t_size())
    ASSERT(t_lookup(1))
    ASSERT(t_lookup(2))
    ASSERT(t_lookup(3))
    t_add(4);
    ASSERT(4 == t_size())
    ASSERT(t_lookup(1))
    ASSERT(t_lookup(2))
    ASSERT(t_lookup(3))
    ASSERT(t_lookup(4))
    t_add(5);
    ASSERT(4 == t_size())
    ASSERT(t_lookup(2))
    ASSERT(t_lookup(3))
    ASSERT(t_lookup(4))
    ASSERT(t_lookup(5))
    t_remove(6);
    ASSERT(4 == t_size())
    ASSERT(t_lookup(2))
    ASSERT(t_lookup(3))
    ASSERT(t_lookup(4))
    ASSERT(t_lookup(5))
    t_remove(5);
    ASSERT(3 == t_size())
    ASSERT(t_lookup(2))
    ASSERT(t_lookup(3))
    ASSERT(t_lookup(4))
    t_remove(2);
    ASSERT(2 == t_size())
    ASSERT(t_lookup(3))
    ASSERT(t_lookup(4))
    t_remove(3);
    ASSERT(1 == t_size())
    ASSERT(t_lookup(4))
    t_remove(4);
    ASSERT(0 == t_size())
    t_finish();
}

#endif

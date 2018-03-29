#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tiny_set.h"

int 
tiny_set_init(struct TinySet *ts, size_t capacity, size_t element_size) {
    void *data = malloc(capacity * element_size);
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
    free(ts->data);
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

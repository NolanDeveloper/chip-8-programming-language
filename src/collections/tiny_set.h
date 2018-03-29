/* This file implements set-like data structure. Its implemention is based on
 * array. Most operations have linear complexity and therefore can only be
 * effective while size of set is small. Size is initialization-time constant.
 * If set is full adding one more item removes least recently added one. */
struct TinySet {
    void *data;                 /* array of items */
    size_t element_size;        /* size of single item in bytes */
    size_t size;                /* number of elements array currently contains */
    size_t capacity;            /* maximum number of elements */
};

/* This function initializes ts, allocates capacity * elelemnt_size bytes. 
 * In case of success result is 0. Non 0 value is returned in case of error. */
int tiny_set_init(struct TinySet *ts, size_t capacity, size_t element_size);

/* It frees memory which was allocated in tiny_set_init. */
void tiny_set_free(struct TinySet *ts);

/* It adds element to set ts. If set is full least recently added element gets
 * removed. */
void tiny_set_add(struct TinySet *ts, void *element);

/* It removes element from set. */
void tiny_set_remove(struct TinySet *ts, void *element);

/* It checks if element is in ts set. If it is retuns pointer to it. Otherwise
 * returns NULL. */
void *tiny_set_lookup(struct TinySet *ts, void *element);

/* It returns number of elements currently stored in ts. */
size_t tiny_set_size(struct TinySet *ts);

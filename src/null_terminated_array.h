enum NtaError {
    NTA_OK,
    NTA_OUT_OF_MEMORY,
    NTA_BAD_ARGUMENT,
};

struct NullTerminatedArray {
    void *data;
    void *null;
    size_t element_size;
    size_t capacity;
};

extern enum NtaError null_terminated_array_init(
    struct NullTerminatedArray *nta, size_t capacity, size_t element_size, 
    void *null, void *(*allocate)(size_t));
extern void null_terminated_array_free(struct NullTerminatedArray *nta, void (*free)(void *));
extern void null_terminated_array_add(struct NullTerminatedArray *nta, void *element);
extern void null_terminated_array_remove(struct NullTerminatedArray *nta, void *element);
extern bool null_terminated_array_is_empty(struct NullTerminatedArray *nta);
extern size_t null_terminated_array_size(struct NullTerminatedArray *nta);
extern void *null_terminated_array_at(struct NullTerminatedArray *nta, size_t i);

#define ASSERT_MSG(c, ...) \
    if (!(c)) { \
        printf(__FILE__ ":%i: Assertion failed \"" #c "\"\n", __LINE__); \
        printf(__VA_ARGS__); \
        exit(1); \
    }

uint_fast32_t string_hash(const char *str);

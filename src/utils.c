#include <stdint.h>

#include "utils.h"

uint_fast32_t
string_hash(const char *str) {
    uint_fast32_t hash = 5381, c;
    while ((c = *(const unsigned char *)str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}


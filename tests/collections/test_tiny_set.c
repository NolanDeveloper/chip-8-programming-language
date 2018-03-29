#include "tiny_set.c"

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include "utils.h"

struct TinySet ts;

void    t_init(void)       { tiny_set_init(&ts, 4, sizeof(int)); }
void    t_finish(void)     { tiny_set_free(&ts); }
void    t_add(int x)       { tiny_set_add(&ts, &x); }
void    t_remove(int x)    { tiny_set_remove(&ts, &x); }
size_t  t_size()           { return tiny_set_size(&ts); }
bool    t_lookup(int x)    { return tiny_set_lookup(&ts, &x); }

void t_check(size_t n, ...) { 
    va_list args;
    ASSERT_MSG(n == t_size(), "\tn = %zu\n\tt_size() = %zu\n", n, t_size())
    va_start(args, n);
    for (size_t i = 0; i < n; ++i) {
        int x = va_arg(args, int);
        ASSERT_MSG(t_lookup(x), "x = %i\n", x)
    }
    va_end(args);
}

int main() {
    t_init();       t_check(0);
    t_add(1);       t_check(1, 1);
    t_add(2);       t_check(2, 1, 2);
    t_add(3);       t_check(3, 1, 2, 3);
    t_add(4);       t_check(4, 1, 2, 3, 4);
    t_add(5);       t_check(4, 2, 3, 4, 5);
    t_remove(6);    t_check(4, 2, 3, 4, 5);
    t_remove(5);    t_check(3, 2, 3, 4);
    t_remove(2);    t_check(2, 3, 4);
    t_add(7);       t_check(3, 3, 4, 7);
    t_remove(3);    t_check(2, 4, 7);
    t_remove(4);    t_check(1, 7);
    t_remove(7);    t_check(0);
    t_remove(7);    t_check(0);
    t_finish();
}

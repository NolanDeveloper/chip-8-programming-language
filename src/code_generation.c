#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include "utils.h"
#include "c8asm.h"
#include "code_generation.h"

extern void
cg_emit_label(const char *label) {
    asm_emit_label(label);
}

extern void
cg_emit_call_label(const char *label) {
    asm_emit_call_label(label);
}

/* ToDo: find out if it's better to write implementation which doesn't do
 * unnecessary operations. It will require writing loop instead of just using
 * standard functions which may be even slower because unlikely it will be as
 * optimised as standard functions which most probably use SIMD instructions. */
static void
null_terminated_array_add(void *array, const void *x, const void *null,
                          size_t number_of_members, size_t member_size) {
    memmove((char*)array + member_size, array, (number_of_members - 1) * member_size);
    memcpy((char*)array + (number_of_members - 1) * member_size, null, member_size);
    memcpy(array, x, member_size);
}

static void
null_terminated_array_remove(void *array, const void *x, const void *null, size_t member_size) {
    char *in, *out;
    for (in = array, out = array; memcmp(in, null, member_size); in += member_size) {
        if (!memcmp(in, x, member_size)) continue;
        memcpy(in, out, member_size);
        out += member_size;
    }
    memcpy(out, null, member_size);
}

enum Register {
    V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, VA, VB, VC, VD, VE, VF,
    REGISTER_MAX,
};
static enum Register null_register = V0;

#define DESCRIPTOR_SIZE     4

/* Registers V0 and VF are special.
 * V0 is only used for store and load operations.
 * VF is only used as flag register for some instructions. */
struct AddressDescriptor {
    bool            is_in_memory; /* Can actual value of variable be found in memory? */
    enum Register   registers[DESCRIPTOR_SIZE];
                                  /* Registers which hold actual value of the variable.
                                   * 0 is end of list. */
};

static void
address_descriptor_add_register(struct AddressDescriptor *ad, enum Register r) {
    null_terminated_array_add(
        ad->registers, &r, &null_register, DESCRIPTOR_SIZE, sizeof(enum Register));
}

struct Variable {
    char                        *name;
    struct AddressDescriptor    descriptor;
};

#define MAX_VARIABLES               512

struct RegisterDescriptor {
    size_t variables[DESCRIPTOR_SIZE]; /* Variables which hold actual value in the register.
                                        * MAX_VARIABLES is end of list. */
};
static size_t null_variable = MAX_VARIABLES;

static void
register_descriptor_add_variable(struct RegisterDescriptor *rd, size_t x) {
    null_terminated_array_add(
        rd->variables, &x, &null_variable, DESCRIPTOR_SIZE, sizeof(size_t));
}

static struct Variable              variables[MAX_VARIABLES];
static size_t                       number_of_variables;
static struct RegisterDescriptor    registers[REGISTER_MAX];

#define TABLE_SIZE (MAX_VARIABLES * 5 / 4)
struct Entry {
    uint_fast32_t   hash;
    char            *name;
    size_t          variable;
} symbols[TABLE_SIZE];
static size_t number_of_symbols;

extern void
cg_init(void) {
    asm_init();
    number_of_variables = 0;
    for (enum Register r = V1; r <= VE; ++r) {
        registers[r].variables[0] = null_variable;
    }
    number_of_symbols = 0;
    for (size_t i = 0; i < TABLE_SIZE; ++i) {
        symbols[i].name = NULL;
    }
}

static void
add_symbol(char *name, size_t variable) {
    if (number_of_symbols >= MAX_VARIABLES) {
        fprintf(stderr, "Too many variables.");
        exit(1);
    }
    uint_fast32_t hash  = string_hash(name);
    size_t i, n         = TABLE_SIZE;
    for (i = hash % n; symbols[i].name; ++i) {
        assert(hash != symbols[i].hash || strcmp(name, symbols[i].name));
    }
    symbols[i].hash         = hash;
    symbols[i].name         = name;
    symbols[i].variable     = variable;
}

extern bool
cg_lookup(char *name, size_t *variable) {
    uint_fast32_t hash = string_hash(name);
    size_t n = TABLE_SIZE;
    for (size_t i = hash % n; symbols[i].name; ++i) {
        if (hash == symbols[i].hash && !strcmp(name, symbols[i].name)) {
            *variable = symbols[i].variable;
            return true;
        }
    }
    return false;
}

static char *
alloc_printf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    size_t l = vsnprintf(NULL, 0, format, ap);
    char *string = malloc(l + 1);
    vsnprintf(string, l + 1, format, ap);
    va_end(ap);
    return string;
}

extern size_t
cg_make_variable(char *name) {
    if (number_of_variables >= MAX_VARIABLES) {
        fprintf(stderr, "Too many variables.\n");
        exit(1);
    }
    size_t v = number_of_variables++;
    if (name) {
        variables[v].name = name;
        add_symbol(name, v);
    } else {
        variables[v].name = alloc_printf("~%zx", v);
    }
    variables[v].descriptor.is_in_memory = true;
    variables[v].descriptor.registers[0] = null_register;
    return v;
}

static void
generate_store(enum Register r, size_t x) {
    /* Save value of variable x from register r into memory with spectacular
     * efficiency. */
    asm_emit_ld_vx_vy(V0, r);
    asm_emit_ld_i_label(variables[x].name);
    asm_emit_ld_ii_vx(V0);
    /* Update descriptor. */
    variables[x].descriptor.is_in_memory = true;
}

static void
generate_load(size_t x, enum Register r) {
    /* Load value of variable x from memory into register x with spectacular
     * efficiency. */
    asm_emit_ld_i_label(variables[x].name);
    asm_emit_ld_vx_ii(V0);
    asm_emit_ld_vx_vy(V0, r);
    /* Update descriptors. */
    registers[r].variables[0] = x;
    registers[r].variables[1] = null_variable;
    address_descriptor_add_register(&variables[x].descriptor, r);
}

static void
get_register(size_t x, enum Register *rx) {
    /* If x is alreay in some register return this register. */
    struct AddressDescriptor *d = &variables[x].descriptor;
    if (null_register != d->registers[0]) {
        *rx = d->registers[0];
        for (size_t i = 0; null_register != d->registers[i]; ++i) {
            d->registers[i] = d->registers[i + 1];
        }
        return;
    }
    /* Find register which holds least number of variables. */
    enum Register   min_r;
    size_t          min_count = DESCRIPTOR_SIZE;
    for (enum Register r = V1; r <= VE && min_count; ++r) {
        size_t count = 0;
        while (null_variable != registers[r].variables[count]) ++count;
        if (min_count < count) continue;
        min_count   = count;
        min_r       = r;
    }
    *rx = min_r;
    /* Make sure values of the variables in this register aren't lost. */
    if (!min_count) return;
    for (size_t v = 0; null_variable != registers[min_r].variables[v]; ++v) {
        /* If v resides somewhere else it's not lost. */
        d = &variables[x].descriptor;
        if (d->is_in_memory) continue;
        if (d->registers[0] != min_r) continue;
        if (null_register != d->registers[1] && min_r != d->registers[1]) continue;
        /* ToDo: Two pass compiler could check if v is not used anymore to
         * avoid generating unnecessary stores. */
        generate_store(min_r, x);
    }
}

static bool
is_in_register(size_t x, enum Register r) {
    size_t v, i = 0;
    for (;;) {
        v = registers[r].variables[i];
        if (null_variable == v) return false;
        if (x == v) return true;
        ++i;
    }
    fprintf(stderr, "unreachable statement");
    exit(1);
}

extern void
cg_emit_operation(enum OperationType operation, size_t x, size_t y) {
    enum Register rx, ry;
    if (OP_ASSIGN == operation) {
        get_register(y, &ry);
        if (!is_in_register(y, ry)) generate_load(y, ry);
        /* Update descriptors. */
        register_descriptor_add_variable(&registers[ry], x);
        variables[x].descriptor.is_in_memory = false;
        variables[x].descriptor.registers[0] = ry;
        variables[x].descriptor.registers[1] = null_register;
    } else {
        get_register(x, &rx);
        get_register(y, &ry);
        if (!is_in_register(x, rx)) generate_load(x, rx);
        if (!is_in_register(y, ry)) generate_load(y, ry);
        switch (operation) {
        default:
            fprintf(stderr, "unknown operation: %i", (int)operation);
            exit(1);
        case OP_ADD:        asm_emit_add_vx_vy(rx, ry); break;
        case OP_SUBTRACT:   asm_emit_sub_vx_vy(rx, ry); break;
        }
        /* Update descriptors. */
        registers[rx].variables[0] = x;
        registers[rx].variables[1] = null_variable;
        variables[x].descriptor.is_in_memory = false;
        variables[x].descriptor.registers[0] = rx;
        variables[x].descriptor.registers[1] = null_register;
        for (size_t i = 0; i < number_of_variables; ++i) {
            if (x == i) continue;
            void *array         = variables[x].descriptor.registers;
            enum Register null  = null_register;
            null_terminated_array_remove(array, &rx, &null, sizeof(enum Register));
        }
    }
}

extern void
cg_emit_assign_constant(size_t x, uint_fast16_t constant) {
    enum Register rx;
    get_register(x, &rx);
    asm_emit_ld_vx_byte(rx, constant);
    /* Update descriptor. */
    registers[rx].variables[0] = x;
    registers[rx].variables[1] = null_variable;
    variables[x].descriptor.is_in_memory = false;
    variables[x].descriptor.registers[0] = rx;
    variables[x].descriptor.registers[1] = null_register;
}

extern void
cg_emit_bss(void) {
    for (size_t i = 0; i < number_of_variables; ++i) {
        asm_emit_label(variables[i].name);
        asm_emit_data(0);
    }
}

extern void
cg_get_rom(const void **rom, size_t *size) {
    *rom     = asm_get_machine_code();
    *size    = asm_get_instruction_pointer();
}

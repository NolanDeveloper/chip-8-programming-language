%include {

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limits.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "utils.h"
#include "lexer.h"
#include "c8asm.h"

static uint_fast32_t    line = 0;
static uint_fast32_t    number_of_errors = 0;

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
    char *in = array, *out = array;
    do {
        if (!memcmp(in, x, member_size)) continue;
        memcpy(in, out, member_size);
    } while (memcmp(in, null, member_size));
}

#define DESCRIPTOR_SIZE     4

enum Register {
    V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, VA, VB, VC, VD, VE, VF,
    REGISTER_MAX,
    NULL_REGISTER = V0,
};

/* Registers V0 and VF are special.
 * V0 is only used for store and load operations.
 * VF is only used as flag register for some instructions. */
struct AddressDescriptor {
    bool            is_in_memory; /* Can actual value of variable be found in memory. */
    enum Register   registers[DESCRIPTOR_SIZE];
                                  /* Registers which hold actual value of the variable.
                                   * 0 is end of list. */
};

static void
address_descriptor_add_register(struct AddressDescriptor *ad, enum Register r) {
    enum Register null = NULL_REGISTER;
    null_terminated_array_add(ad->registers, &r, &null, DESCRIPTOR_SIZE, sizeof(enum Register));
}

struct Variable {
    char                        *name;
    struct AddressDescriptor    descriptor;
};

#define MAX_VARIABLES               512
#define NULL_VARIABLE               MAX_VARIABLES

struct RegisterDescriptor {
    size_t variables[DESCRIPTOR_SIZE]; /* Variables which hold actual value in the register.
                                        * MAX_VARIABLES is end of list. */
};

static void
register_descriptor_add_variable(struct RegisterDescriptor *rd, size_t x) {
    size_t null = NULL_VARIABLE;
    null_terminated_array_add(rd->variables, &x, &null, DESCRIPTOR_SIZE, sizeof(size_t));
}

static struct Variable              variables[MAX_VARIABLES];
static size_t                       number_of_variables = 0;
static struct RegisterDescriptor    registers[REGISTER_MAX];

#define TABLE_SIZE (MAX_VARIABLES * 5 / 4)
struct Entry {
    uint_fast32_t   hash;
    char            *name;
    size_t          variable;
} symbols[TABLE_SIZE];

static void
add_symbol(char *name, size_t variable) {
    uint_fast32_t hash  = string_hash(name);
    size_t i, n         = TABLE_SIZE;
    for (i = hash % n; symbols[i].name; ++i) {
        assert(hash != symbols[i].hash || strcmp(name, symbols[i].name));
    }
    symbols[i].hash         = hash;
    symbols[i].name         = name;
    symbols[i].variable     = variable;
}

static bool
lookup_symbol(char *name, size_t *variable) {
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

static size_t
make_variable(char *name) {
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
    variables[v].descriptor.registers[0] = NULL_REGISTER;
    return v;
}

static void
generate_store(enum Register r, size_t x) {
    /* Save value of variable x from register r into memory with spectacular
     * efficiency. */
    asm_emit_ld_vx_vy(r, V0);
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
    asm_emit_ld_ii_vx(V0);
    asm_emit_ld_vx_vy(V0, r);
    /* Update descriptors. */
    registers[r].variables[0] = x;
    registers[r].variables[1] = NULL_VARIABLE;
    address_descriptor_add_register(&variables[x].descriptor, r);
}

static void
get_register(size_t x, enum Register *rx) {
    /* If x is alreay in some register return this register. */
    struct AddressDescriptor *d = &variables[x].descriptor;
    if (NULL_REGISTER != d->registers[0]) {
        *rx = d->registers[0];
        for (size_t i = 0; NULL_REGISTER != d->registers[i]; ++i) {
            d->registers[i] = d->registers[i + 1];
        }
        return;
    }
    /* Find register which holds least number of variables. */
    enum Register   min_r;
    size_t          min_count = DESCRIPTOR_SIZE;
    for (enum Register r = V1; r <= VE && min_count; ++r) {
        size_t count = 0;
        while (NULL_VARIABLE != registers[r].variables[count]) ++count;
        if (min_count < count) continue;
        min_count   = count;
        min_r       = r;
    }
    *rx = min_r;
    /* Make sure values of the variables in this register aren't lost. */
    if (!min_count) return;
    for (size_t v = 0; NULL_VARIABLE != registers[min_r].variables[v]; ++v) {
        /* If v resides somewhere else it's not lost. */
        d = &variables[x].descriptor;
        if (d->is_in_memory) continue;
        if (d->registers[0] != min_r) continue;
        if (NULL_REGISTER != d->registers[1] && min_r != d->registers[1]) continue;
        /* Two pass compiler could check if v is not used anymore to avoid
         * generating unnecessary stores. But I'm too lazy and this sounds like
         * a work. */
        generate_store(min_r, x);
    }
}

/* ToDo: add other arithmetic operations */
enum OperationType {
    OP_ASSIGN,
    OP_ADD,
    OP_SUBTRACT,
};

static bool
is_in_register(size_t x, enum Register r) {
    size_t v, i = 0;
    for (;;) {
        v = registers[r].variables[i];
        if (NULL_VARIABLE == v) return false;
        if (x == v) return true;
        ++i;
    }
    fprintf(stderr, "unreachable statement");
    exit(1);
}

static void
generate_operation(enum OperationType operation, size_t x, size_t y) {
    enum Register rx, ry;
    if (OP_ASSIGN == operation) {
        get_register(y, &ry);
        if (!is_in_register(y, ry)) generate_load(y, ry);
        /* Update registers. */
        register_descriptor_add_variable(&registers[ry], x);
        variables[x].descriptor.is_in_memory = false;
        variables[x].descriptor.registers[0] = ry;
        variables[x].descriptor.registers[1] = NULL_REGISTER;
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
        registers[rx].variables[1] = NULL_VARIABLE;
        variables[x].descriptor.is_in_memory = false;
        variables[x].descriptor.registers[0] = rx;
        variables[x].descriptor.registers[1] = NULL_REGISTER;
        for (size_t i = 0; i < number_of_variables; ++i) {
            if (x == i) continue;
            void *array         = variables[x].descriptor.registers;
            enum Register null  = NULL_REGISTER;
            null_terminated_array_remove(array, &rx, &null, sizeof(enum Register));
        }
    }
}

static void
generate_initialization_with_constant(size_t x, uint_fast16_t constant) {
    enum Register rx;
    get_register(x, &rx);
    asm_emit_ld_vx_byte(rx, constant);
    /* Update descriptor. */
    registers[rx].variables[0] = x;
    registers[rx].variables[1] = NULL_VARIABLE;
    variables[x].descriptor.is_in_memory = false;
    variables[x].descriptor.registers[0] = rx;
    variables[x].descriptor.registers[1] = NULL_REGISTER;
}

} /* end %include */

%token_type { union TokenData }

%left PLUS_SIGN MINUS_SIGN.

%syntax_error {
        ++number_of_errors;
        fprintf(stderr, "line %"PRIuFAST32": syntax error\n", line);
    }

start ::= unit.

unit ::= unit functionDeclaration.
unit ::= functionDeclaration.

functionName ::= IDENTIFIER(A). { asm_emit_label(A.sValue); }

functionDeclaration ::= KW_FUNCTION functionName compoundStatement.

statement ::= assignmentStatement.
statement ::= compoundStatement.
statement ::= callStatement.

assignmentStatement ::= IDENTIFIER(A) EQUALS_SIGN expression(B) SEMICOLON. {
        size_t variable = make_variable(A.sValue);
        generate_operation(OP_ASSIGN, variable, B);
    }

compoundStatement ::= LEFT_BRACES listOfStatements0 RIGHT_BRACES.

callStatement ::= KW_CALL IDENTIFIER(A) SEMICOLON. { asm_emit_call_label(A.sValue); }

listOfStatements0 ::= listOfStatements1.
listOfStatements0 ::= .

listOfStatements1 ::= listOfStatements1 SEMICOLON statement.
listOfStatements1 ::= statement.

%type expression { size_t }
expression(A) ::= LEFT_PARENTHESES expression(B) RIGHT_PARENTHESES. { A = B; }
expression(A) ::= expression(B) PLUS_SIGN expression(C). {
        A = make_variable(NULL);
        generate_operation(OP_ASSIGN, A, B);
        generate_operation(OP_ADD, A, C);
    }
expression(A) ::= expression(B) MINUS_SIGN expression(C). {
        A = make_variable(NULL);
        generate_operation(OP_ASSIGN, A, B);
        generate_operation(OP_SUBTRACT, A, C);
    }
expression(A) ::= IDENTIFIER(B). {
        bool exist = lookup_symbol(B.sValue, &A);
        if (!exist) {
            ++number_of_errors;
            fprintf(stderr, "line %"PRIuFAST32": symbol \"%.20s\" doesn't exist\n",
                    line, B.sValue);
            A = make_variable(NULL); /* Create stub. */
        }
    }
expression(A) ::= INTEGER(B). {
        A = make_variable(NULL);
        generate_initialization_with_constant(A, B.iValue);
    }

%code {

static size_t
get_file_size(const char *path) {
    struct stat st;
    if (stat(path, &st)) {
        fprintf(stderr, "Can't get size of file: %s", strerror(errno));
        exit(1);
    }
    if (st.st_size < 0) {
        fprintf(stderr, "File has negative size");
        exit(1);
    }
    return (size_t)st.st_size;
}

static void
load_file(const char *path, char *buffer, size_t buffer_size) {
    size_t size = get_file_size(path);
    if (buffer_size < size) {
        fprintf(stderr, "File is too big.");
        exit(1);
    }
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Can't open file: %s", strerror(errno));
        exit(1);
    }
    if (size != fread(buffer, 1, size, f)) {
        fprintf(stderr, "Can't read file: %s", strerror(errno));
        exit(1);
    }
    fclose(f);
    buffer[size] = 0;
}

#define KB              1024
#define MB              (1024 * KB)
#define BUFFER_SIZE     (4 * MB)
static char buffer[BUFFER_SIZE];

int main(int argc, char *argv[]) {
    if (3 != argc) {
        fprintf(stderr, "usage: %s input.c8 output.dat\n", argv[0]);
        exit(0);
    }
    load_file(argv[1], buffer, BUFFER_SIZE);
    char *cursor = buffer;
    void *parse = ParseAlloc(malloc);
    struct Token token;
    while (0 < (token.type = lexer_next_token(&cursor, &token.data, &line))) {
        Parse(parse, token.type, token.data);
    }
    if (token.type < 0) {
        fprintf(stderr, "unknown token: %.5s\n", cursor);
        exit(1);
    }
    Parse(parse, 0, token.data);
    return 0;
}

} /* end %code */

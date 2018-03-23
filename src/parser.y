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
#include "code_generation.h"

static uint_fast32_t    line = 1;
static uint_fast32_t    number_of_errors = 0;
static const char       *output_file_path;

static void
save_file(const char *path, const char *buffer, size_t size) {
    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "Can't open file %s: %s\n", path, strerror(errno));
        exit(1);
    }
    if (size != fwrite(buffer, 1, size, f)){
        fprintf(stderr, "Can't write to file %s: %s\n", path, strerror(errno));
        exit(1);
    }
    fclose(f);
}

} /* end %include */

%token_type { union TokenData }

%left PLUS_SIGN MINUS_SIGN.

%syntax_error {
        ++number_of_errors;
        fprintf(stderr, "line %"PRIuFAST32": syntax error\n", line);
    }

%parse_accept {
        if (number_of_errors) {
            fprintf(stderr, "There were %"PRIuFAST32" errors", number_of_errors);
            exit(1);
        } else {
            cg_emit_bss();
            const void *rom;
            size_t size;
            cg_get_rom(&rom, &size);
            save_file(output_file_path, rom, size);
        }
    }

start ::= unit.

unit ::= unit functionDeclaration.
unit ::= functionDeclaration.

functionName ::= IDENTIFIER(A). { cg_emit_label(A.sValue); }

functionDeclaration ::= KW_FUNCTION functionName compoundStatement.

statement ::= assignmentStatement.
statement ::= compoundStatement.
statement ::= callStatement.

assignmentStatement ::= IDENTIFIER(A) EQUALS_SIGN expression(B) SEMICOLON. {
        size_t variable;
        if (!cg_lookup(A.sValue, &variable)) {
            variable = cg_make_variable(A.sValue);
        }
        cg_emit_operation(OP_ASSIGN, variable, B);
    }

compoundStatement ::= LEFT_BRACES listOfStatements0 RIGHT_BRACES.

listOfStatements0 ::= listOfStatements1.
listOfStatements0 ::= .

listOfStatements1 ::= listOfStatements1 statement.
listOfStatements1 ::= statement.

callStatement ::= KW_CALL IDENTIFIER(A) SEMICOLON. { cg_emit_call_label(A.sValue); }

%type expression { size_t }
expression(A) ::= LEFT_PARENTHESES expression(B) RIGHT_PARENTHESES. { A = B; }
expression(A) ::= expression(B) PLUS_SIGN expression(C). {
        A = cg_make_variable(NULL);
        cg_emit_operation(OP_ASSIGN, A, B);
        cg_emit_operation(OP_ADD, A, C);
    }
expression(A) ::= expression(B) MINUS_SIGN expression(C). {
        A = cg_make_variable(NULL);
        cg_emit_operation(OP_ASSIGN, A, B);
        cg_emit_operation(OP_SUBTRACT, A, C);
    }
expression(A) ::= IDENTIFIER(B). {
        bool exist = cg_lookup(B.sValue, &A);
        if (!exist) {
            ++number_of_errors;
            fprintf(stderr, "line %"PRIuFAST32": symbol \"%.20s\" doesn't exist\n",
                    line, B.sValue);
            A = cg_make_variable(NULL); /* Create stub. */
        }
    }
expression(A) ::= INTEGER(B). {
        A = cg_make_variable(NULL);
        cg_emit_assign_constant(A, B.iValue);
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
        fprintf(stderr, "File is too big.\n");
        exit(1);
    }
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Can't open file %s: %s\n", path, strerror(errno));
        exit(1);
    }
    if (size != fread(buffer, 1, size, f)) {
        fprintf(stderr, "Can't read file %s: %s\n", path, strerror(errno));
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
    output_file_path = argv[2];
    char *cursor = buffer;
    void *parse = ParseAlloc(malloc);
    struct Token token;
    cg_init();
    cg_emit_call_label("main");
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "utils.h"

/* string - string started with digit and not necessarily ended with zero byte. */
static uint_fast16_t
strtoi(const char *string, int base) {
    char *endptr;
    unsigned long value = strtoul(string, &endptr, base);
    assert(endptr != string);
    if (UINT_FAST16_MAX < value) {
        int l = endptr - string;
        if (l < 20) {
            fprintf(stderr, "%.*s is too big number", l, string);
        } else {
            fprintf(stderr, "%.20s... is too big number", string);
        }
        exit(1);
    }
    return (uint_fast16_t)value;
}

extern int
lexer_next_token(char **cursor, union TokenData *data, uint_fast32_t *line) {
restart:;
    char *token = *cursor;
    (void)token;
    /*!re2c
    re2c:define:YYCTYPE     = char;
    re2c:define:YYCURSOR    = *cursor;
    re2c:yyfill:enable      = 0;
    letter      = [a-zA-Z];
    decDigit    = [0-9];
    hexDigit    = (decDigit | [a-zA-Z]);
    *           { return -1; }
    "\x00"      { return 0; }
    [ \t]+      { goto restart; }
    "\n"        { ++*line; goto restart; }
    "+"         { return PLUS_SIGN; }
    "-"         { return MINUS_SIGN; }
    "function"  { return KW_FUNCTION; }
    "call"      { return KW_CALL; }
    "("         { return LEFT_PARENTHESES; }
    ")"         { return RIGHT_PARENTHESES; }
    ";"         { return SEMICOLON; }
    "="         { return EQUALS_SIGN; }
    "{"         { return LEFT_BRACES; }
    "}"         { return RIGHT_BRACES; }
    letter (letter | decDigit)* {
            ptrdiff_t l = *cursor - token;
            assert(0 <= l);
            if (20 < l) {
                // Better return error code
                fprintf(stderr, "Too long identifier");
                exit(1);
            }
            char *name = malloc((size_t)l + 1);
            strncpy(name, token, (size_t)l);
            data->sValue = name;
            return IDENTIFIER;
        }
    decDigit+ {
            data->iValue = strtoi(token, 10);
            return INTEGER;
        }
    */
}

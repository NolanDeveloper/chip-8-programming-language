#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "utils.h"

#if 0
/* string - string started with digit and not necessarily ended with zero byte. */
static int
strtoi(const char *string, char **endptr, int base) {
    char *endptr1;
    long value = strtol(string, &endptr1, base);
    if (endptr) *endptr = endptr1;
    assert(endptr1 != string);
    if (INT_MAX < value || value < INT_MIN) {
        int l = endptr1 - string;
        if (l < 20) {
            die("%.*s is too big number", l, string);
        } else {
            die("%.20s... is too big number", string);
        }
    }
    return (int)value;
}
#endif

extern int
lexer_next_token(char **cursor, union TokenData *data, uint_fast32_t *line) {
    (void)data;
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
    letter (letter | decDigit)*
                { return IDENTIFIER; }
    decDigit+   { return INTEGER; }
    */
}

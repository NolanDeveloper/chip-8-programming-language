%include {

#include <stdlib.h>
#include <stdint.h>

#include "utils.h"
#include "lexer.h"

static uint_fast32_t line = 1;

} /* end %include */

%token_type { union TokenData }

%left PLUS_SIGN MINUS_SIGN.
%right UNARY_MINUS.

start ::= unit.

unit ::= unit functionDeclaration.
unit ::= functionDeclaration.

functionDeclaration ::= KW_FUNCTION IDENTIFIER functionParameters compoundStatement.

functionParameters ::= LEFT_PARENTHESES listOfIdentifiers0 RIGHT_PARENTHESES.

listOfIdentifiers0 ::= listOfIdentifiers1.
listOfIdentifiers0 ::= .

listOfIdentifiers1 ::= listOfIdentifiers1 COMMA IDENTIFIER.
listOfIdentifiers1 ::= IDENTIFIER.

statement ::= emptyStatement.
statement ::= assignmentStatement.
statement ::= compoundStatement.
statement ::= returnStatement.

emptyStatement ::= SEMICOLON.

assignmentStatement ::= IDENTIFIER EQUALS_SIGN expression SEMICOLON.

compoundStatement ::= LEFT_BRACES listOfStatements0 RIGHT_BRACES.

returnStatement ::= KW_RETURN expression.

listOfStatements0 ::= listOfStatements1.
listOfStatements0 ::= .

listOfStatements1 ::= listOfStatements1 SEMICOLON statement.
listOfStatements1 ::= statement.

expression ::= expression PLUS_SIGN expression.
expression ::= expression MINUS_SIGN expression.
expression ::= MINUS_SIGN expression. [UNARY_MINUS]
expression ::= LEFT_PARENTHESES expression RIGHT_PARENTHESES.
expression ::= IDENTIFIER LEFT_PARENTHESES listOfExpressions0 RIGHT_PARENTHESES.
expression ::= IDENTIFIER.
expression ::= INTEGER.

listOfExpressions0 ::= listOfExpressions1.
listOfExpressions0 ::= .

listOfExpressions1 ::= listOfExpressions1 COMMA expression.
listOfExpressions1 ::= expression.

%code {

int main(int argc, char *argv[]) {
    if (3 != argc) {
        printf("usage: %s input.c8 output.dat\n", argv[0]);
        exit(0);
    }
    char *cursor = loadFile(argv[1]);
    void *parse = ParseAlloc(emalloc);
    struct Token token;
    while ((token.type = lexerNextToken(&cursor, &token.data, &line))) {
        Parse(parse, token.type, token.data);
    }
    Parse(parse, 0, token.data);
    return 0;
}

} /* end %code */

union TokenData {
    int iValue;
    char *sValue;
};

struct Token {
    int type;
    union TokenData data;
};

extern int lexerNextToken(char **cursor, union TokenData *data, uint_fast32_t *line);

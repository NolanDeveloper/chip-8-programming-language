union TokenData {
    uint_fast16_t iValue;
    char *sValue;
};

struct Token {
    int type;
    union TokenData data;
};

int lexer_next_token(char **cursor, union TokenData *data, uint_fast32_t *line);
void lexer_free_token(struct Token *token);

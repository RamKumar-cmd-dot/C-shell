#ifndef LEXER_H
#define LEXER_H

typedef enum{
    WORD,
    OP_PIPE,
    OP_LT,
    OP_GT,
    OP_GTGT,
    OP_SEMI,
    OP_AMP
}TokenType;

typedef struct{
    TokenType type;
    char* value;
}Token;

Token* lexer(char *input, int *count);

#endif
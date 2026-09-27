#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"

int parse(Token* tokens, int count);
int parse_line(Token *tokens, int* pos, int count);
int parse_cmd(Token *tokens, int* pos, int count);
int parse_tgt(Token *tokens, int* pos, int count);
int parse_bg(Token *tokens, int* pos, int count);
int parse_arg(Token *tokens, int* pos, int count);

#endif
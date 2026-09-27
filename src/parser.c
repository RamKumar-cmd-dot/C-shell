#include "../include/main.h"

int parse_arg(Token* tokens, int* pos, int count){
    if(*pos==count) return 1;
    if(tokens[*pos].type==WORD){
        (*pos)++;
        return parse_arg(tokens, pos, count);
    }
    else if(tokens[*pos].type==OP_LT || tokens[*pos].type==OP_GT || tokens[*pos].type==OP_GTGT){
        (*pos)++;
        return parse_tgt(tokens, pos, count);
    }
    else if(tokens[*pos].type==OP_PIPE || tokens[*pos].type==OP_SEMI){
        (*pos)++;
        return parse_cmd(tokens, pos, count);
    }
    else if(tokens[*pos].type==OP_AMP){
        (*pos)++;
        return parse_bg(tokens, pos, count);
    }
    else return 0;
}

int parse_line(Token* tokens, int* pos, int count){
    if(*pos==count) return 1;
    if(tokens[*pos].type!=WORD) return 0;
    (*pos)++;
    return parse_arg(tokens, pos, count);
}

int parse_cmd(Token* tokens, int* pos, int count){
    if(*pos>=count || tokens[*pos].type!=WORD) return 0;
    (*pos) ++;
    return parse_arg(tokens, pos, count);
}

int parse_tgt(Token* tokens, int* pos, int count){
    if(*pos>=count || tokens[*pos].type!=WORD) return 0;
    (*pos) ++;
    return parse_arg(tokens, pos, count);
}

int parse_bg(Token* tokens, int* pos, int count){
    if(*pos==count) return 1;
    if(tokens[*pos].type!=WORD) return 0;
    (*pos)++;
    return parse_arg(tokens, pos, count);
}

int parse(Token* tokens, int count){
    int pos = 0;
    if(!parse_line(tokens, &pos, count)) return 0;
    if(pos==count) return 1;
    else return 0; 
}